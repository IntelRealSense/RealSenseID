// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "FaceAuthenticatorImpl.h"
#include "Logger.h"
#include "PacketManager/Timer.h"
#include "PacketManager/PacketSender.h"
#include "PacketManager/SerialPacket.h"
#include "StatusHelper.h"
#include "RealSenseID/AuthConfig.h"
#include "string.h"
#include <cassert>
#include <cstdint>

#ifdef _WIN32
#include "PacketManager/WindowsSerial.h"
#pragma warning(push)
#pragma warning(disable : 4996) // suppress msvc's strncpy/memcpy warnings
#elif LINUX
#include "PacketManager/LinuxSerial.h"
#else
#error "Platform not supported"
#endif //  _WIN32

static const char* LOG_TAG = "FaceAuthenticatorImpl";

namespace RealSenseID
{
    // save callback functions to use in the secure session later
    FaceAuthenticatorImpl::FaceAuthenticatorImpl(SignatureCallback* callback) :
        _session{ [callback](const unsigned char* buffer, const unsigned int bufferLen, unsigned char* outSig) {
                      return callback->Sign(buffer, bufferLen, outSig);
                  },
                  [callback](const unsigned char* buffer, const unsigned int bufferLen, const unsigned char* sig,
                             const unsigned int sigLen) { return callback->Verify(buffer, bufferLen, sig, sigLen); } }
    {
    }

                  Status FaceAuthenticatorImpl::Connect(const SerialConfig& config)
                  {
                      try
                      {
                          // disconnect if already connected
                          _serial.reset();
                          PacketManager::SerialConfig serial_config;
                          serial_config.ser_type = static_cast<PacketManager::SerialType>(config.serType);
                          serial_config.port = config.port;

#ifdef _WIN32
                          _serial = std::make_unique<PacketManager::WindowsSerial>(serial_config);
#elif LINUX
                          _serial = std::make_unique<PacketManager::LinuxSerial>(serial_config);
#endif
                          return Status::Ok;
                      }
                      catch (const std::exception& ex)
                      {
                          LOG_EXCEPTION(LOG_TAG, ex);
                          return Status::Error;
                      }
                      catch (...)
                      {
                          LOG_ERROR(LOG_TAG, "Unknown exception durting serial connect");
                          return Status::Error;
                      }
                  }

                  void FaceAuthenticatorImpl::Disconnect()
                  {
                      _serial.reset();
                  }

                  Status FaceAuthenticatorImpl::Pair(const char* ecdsaHostPubKey, const char* ecdsaHostPubKeySig, char* ecdsaDevicePubKey)
                  {
                      if (!_serial)
                      {
                          LOG_ERROR(LOG_TAG, "Not connected to a serial port");
                          return Status::Error;
                      }
                      LOG_INFO(LOG_TAG, "Pairing start");

                      unsigned char ecdsaSignedHostPubKey[SIGNED_PUBKEY_SIZE];
                      ::memset(ecdsaSignedHostPubKey, 0, sizeof(ecdsaSignedHostPubKey));
                      ::memcpy(ecdsaSignedHostPubKey, ecdsaHostPubKey, ECC_P256_KEY_SIZE_BYTES);
                      ::memcpy(ecdsaSignedHostPubKey + ECC_P256_KEY_SIZE_BYTES, ecdsaHostPubKeySig, ECC_P256_SIG_SIZE_BYTES);

                      PacketManager::DataPacket packet{ PacketManager::MsgId::HostEcdsaKey, (char*)ecdsaSignedHostPubKey,
                                                        sizeof(ecdsaSignedHostPubKey) };

                      PacketManager::PacketSender sender{ _serial.get() };
                      auto status = sender.SendWithBinary1(packet);
                      if (status != PacketManager::SerialStatus::Ok)
                      {
                          LOG_ERROR(LOG_TAG, "Failed to send ecdsa public key");
                          return ToStatus(status);
                      }

                      status = sender.Recv(packet);
                      if (status != PacketManager::SerialStatus::Ok)
                      {
                          LOG_ERROR(LOG_TAG, "Failed to recv device ecdsa public key");
                          return ToStatus(status);
                      }
                      if (packet.id != PacketManager::MsgId::DeviceEcdsaKey)
                      {
                          LOG_ERROR(LOG_TAG, "Mutual authentication failed");
                          return Status::SecurityError;
                      }

                      assert(IsDataPacket(packet));

                      ::memcpy(ecdsaDevicePubKey, packet.Data().data, ECC_P256_KEY_SIZE_BYTES);

                      DEBUG_SERIAL(LOG_TAG, "Device Pubkey", ecdsaDevicePubKey, ECC_P256_KEY_SIZE_BYTES);
                      LOG_INFO(LOG_TAG, "Pairing Ok");
                      return Status::Ok;
                  }

                  Status FaceAuthenticatorImpl::SetAuthSettings(const RealSenseID::AuthConfig& auth_config)
                  {
                      auto status = _session.Start(_serial.get());
                      if (status != PacketManager::SerialStatus::Ok)
                      {
                          LOG_ERROR(LOG_TAG, "Session start failed with status %d", static_cast<int>(status));
                          return ToStatus(status);
                      }
                      int size = sizeof(auth_config);
                      char settings[2];
                      settings[0] = static_cast<char>(auth_config.camera_rotation);
                      settings[1] = static_cast<char>(auth_config.security_level);
                      PacketManager::DataPacket data_packet{ PacketManager::MsgId::SetAuthSettings, settings, sizeof(settings) };

                      status = _session.SendPacket(data_packet);
                      if (status != PacketManager::SerialStatus::Ok)
                      {
                          LOG_ERROR(LOG_TAG, "Failed sending data packet (status %d)", (int)status);
                      }
                      // convert internal status to api's serial status and return
                      return ToStatus(status);
                  }

                  Status FaceAuthenticatorImpl::QueryAuthSettings(RealSenseID::AuthConfig& auth_config)
                  {
                      auto status = _session.Start(_serial.get());
                      if (status != PacketManager::SerialStatus::Ok)
                      {
                          LOG_ERROR(LOG_TAG, "Session start failed with status %d", static_cast<int>(status));
                          return ToStatus(status);
                      }
                      PacketManager::DataPacket data_packet{ PacketManager::MsgId::GetAuthSettings, NULL, 0 };
                      status = _session.SendPacket(data_packet);
                      if (status != PacketManager::SerialStatus::Ok)
                      {
                          LOG_ERROR(LOG_TAG, "Failed sending data packet (status %d)", (int)status);
                      }

                      PacketManager::DataPacket data_packet_reply{ PacketManager::MsgId::Versioning };
                      status = _session.RecvDataPacket(data_packet_reply);
                      if (status != PacketManager::SerialStatus::Ok)
                      {
                          LOG_ERROR(LOG_TAG, "Failed receiving fa packet (status %d)", (int)status);
                          return ToStatus(status);
                      }

                      if (data_packet_reply.id != PacketManager::MsgId::GetAuthSettings)
                      {
                          LOG_ERROR(LOG_TAG, "Unexpected msg id in reply (%c)", data_packet_reply.id);
                          return Status::Error;
                      }

                      auto data_size = sizeof(data_packet_reply.payload.message.data_msg.data);
                      if (data_size < 2)
                      {
                          return Status::Error;
                      }
                      auth_config.camera_rotation = (AuthConfig::CameraRotation)data_packet_reply.payload.message.data_msg.data[0];
                      auth_config.security_level = (AuthConfig::SecurityLevel)data_packet_reply.payload.message.data_msg.data[1];

                      // convert internal status to api's serial status and return
                      return ToStatus(status);
                  }


                  // Validate given user id.
                  // Return true if valid, false otherwise.
                  bool FaceAuthenticatorImpl::ValidateUserId(const char* user_id)
                  {
                      if (user_id == nullptr)
                      {
                          LOG_ERROR(LOG_TAG, "Invalid user id: nullptr");
                          return false;
                      }

                      auto user_id_len = ::strlen(user_id);
                      bool is_valid = user_id_len > 0 && user_id_len <= PacketManager::MaxUserIdSize;
                      if (!is_valid)
                      {
                          LOG_ERROR(LOG_TAG, "Invalid user id length. Valid size: 1 - %zu", PacketManager::MaxUserIdSize);
                      }
                      return is_valid;
                  }
                  // Do enroll session with the device. Call user's enroll callbacks in the process.
                  // Wait for one of the following to happen:
                  //      We get 'reply' from device ('Y').
                  //      Any non ok status from the session object(i.e. serial comm failed, or session timeout).
                  //      Unexpected msg_id in the fa response.
                  Status FaceAuthenticatorImpl::Enroll(EnrollmentCallback& callback, const char* user_id)
                  {
                      try
                      {
                          if (!ValidateUserId(user_id))
                          {
                              return Status::Error;
                          }
                          auto status = _session.Start(_serial.get());
                          if (status != PacketManager::SerialStatus::Ok)
                          {
                              LOG_ERROR(LOG_TAG, "Session start failed with status %d", static_cast<int>(status));
                              callback.OnResult(ToEnrollStatus(status));
                              return ToStatus(status);
                          }

                          PacketManager::FaPacket fa_packet{ PacketManager::MsgId::Enroll, user_id, 0 };
                          status = _session.SendPacket(fa_packet);
                          if (status != PacketManager::SerialStatus::Ok)
                          {
                              LOG_ERROR(LOG_TAG, "Failed sending fa packet (status %d)", (int)status);
                              callback.OnResult(ToEnrollStatus(status));
                              return ToStatus(status);
                          }

                          PacketManager::Timer session_timer{ _enroll_session_timeout };
                          while (true)
                          {
                              if (session_timer.ReachedTimeout())
                              {
                                  LOG_ERROR(LOG_TAG, "session timeout");
                                  callback.OnResult(EnrollStatus::Failure);
                                  Cancel();
                              }

                              status = _session.RecvFaPacket(fa_packet);
                              if (status != PacketManager::SerialStatus::Ok)
                              {
                                  LOG_ERROR(LOG_TAG, "Failed receiving fa packet (status %d)", (int)status);
                                  callback.OnResult(ToEnrollStatus(status));
                                  return ToStatus(status);
                              }

                              auto msg_id = fa_packet.id;
                              auto fa_status = fa_packet.GetStatusCode();

                              switch (msg_id)
                              {
                                  // end of transaction
                              case (PacketManager::MsgId::Reply):
                                  return Status::Ok;

                              case (PacketManager::MsgId::Result):
                                  callback.OnResult(EnrollStatus(fa_status));
                                  break;

                              case (PacketManager::MsgId::Progress):
                                  callback.OnProgress((FacePose)fa_status);
                                  break;

                              case (PacketManager::MsgId::Hint):
                                  callback.OnHint(EnrollStatus(fa_status));
                                  break;

                              default:
                                  LOG_ERROR(LOG_TAG, "Got unexpected msg id in response: %d", (int)msg_id);
                                  callback.OnResult(EnrollStatus::Failure);
                                  return Status::Error;
                              }
                          }
                      }
                      catch (std::exception& ex)
                      {
                          LOG_EXCEPTION(LOG_TAG, ex);
                          callback.OnResult(EnrollStatus::Failure);
                          return Status::Error;
                      }
                      catch (...)
                      {
                          LOG_ERROR(LOG_TAG, "Unknown exception");
                          callback.OnResult(EnrollStatus::Failure);
                          return Status::Error;
                      }
                  }

                  // Do authenticate session with the device. Call user's authenticate callbacks in the process.
                  // Wait for one of the following to happen:
                  //      We get 'reply' from device ('Y').
                  //      Any non ok status from the session object(i.e. serial comm failed, or session timeout).
                  //      Unexpected msg_id in the fa response.
                  Status FaceAuthenticatorImpl::Authenticate(AuthenticationCallback& callback)
                  {
                      try
                      {
                          auto status = _session.Start(_serial.get());
                          if (status != PacketManager::SerialStatus::Ok)
                          {
                              LOG_ERROR(LOG_TAG, "Session start failed with status %d", static_cast<int>(status));
                              callback.OnResult(ToAuthStatus(status), nullptr);
                              return ToStatus(status);
                          }
                          PacketManager::FaPacket fa_packet{ PacketManager::MsgId::Authenticate };
                          status = _session.SendPacket(fa_packet);
                          if (status != PacketManager::SerialStatus::Ok)
                          {
                              LOG_ERROR(LOG_TAG, "Failed sending fa packet (status %d)", (int)status);
                              callback.OnResult(ToAuthStatus(status), nullptr);
                              return ToStatus(status);
                          }

                          PacketManager::Timer session_timer{ _auth_session_timeout };
                          while (true)
                          {
                              if (session_timer.ReachedTimeout())
                              {
                                  LOG_ERROR(LOG_TAG, "session timeout");
                                  callback.OnResult(AuthenticateStatus::Forbidden, nullptr);
                                  Cancel();
                              }

                              status = _session.RecvFaPacket(fa_packet);
                              if (status != PacketManager::SerialStatus::Ok)
                              {
                                  LOG_ERROR(LOG_TAG, "Failed receiving fa packet (status %d)", (int)status);
                                  callback.OnResult(ToAuthStatus(status), nullptr);
                                  return ToStatus(status);
                              }

                              auto msg_id = fa_packet.id;
                              auto fa_status = fa_packet.GetStatusCode();

                              switch (msg_id)
                              {
                                  // end of transaction
                              case (PacketManager::MsgId::Reply):
                                  return Status::Ok;

                              case (PacketManager::MsgId::Result): {
                                  callback.OnResult(AuthenticateStatus(fa_status), fa_packet.GetUserId());
                                  break;
                              }

                              case (PacketManager::MsgId::Hint):
                                  callback.OnHint(AuthenticateStatus(fa_status));
                                  break;

                              default:
                                  LOG_ERROR(LOG_TAG, "Got unexpected msg id in response: %d", (int)msg_id);
                                  callback.OnHint(AuthenticateStatus::Failure);
                                  return Status::Error;
                              }
                          }
                      }
                      catch (std::exception& ex)
                      {
                          LOG_EXCEPTION(LOG_TAG, ex);
                          callback.OnResult(AuthenticateStatus::Failure, nullptr);
                          return Status::Error;
                      }
                      catch (...)
                      {
                          LOG_ERROR(LOG_TAG, "Unknown exception");
                          callback.OnResult(AuthenticateStatus::Failure, nullptr);
                          return Status::Error;
                      }
                  }

                  // Do infinite authenticate loop with the device. Call user's authenticate callbacks in the process.
                  // Wait for one of the following to happen:
                  //      We get 'reply' from device ('Y' , would happen if "cancel" command was sent by a different thread to this
                  //      device). Any non ok status from the session object(i.e. serial comm failed, or session timeout). Unexpected
                  //      msg_id in the fa response.
                  Status FaceAuthenticatorImpl::AuthenticateLoop(AuthenticationCallback& callback)
                  {
                      try
                      {
                          auto status = _session.Start(_serial.get());
                          if (status != PacketManager::SerialStatus::Ok)
                          {
                              LOG_ERROR(LOG_TAG, "Session start failed with status %d", static_cast<int>(status));
                              callback.OnResult(ToAuthStatus(status), nullptr);
                              return ToStatus(status);
                          }
                          PacketManager::FaPacket fa_packet{ PacketManager::MsgId::AuthenticateLoop };
                          status = _session.SendPacket(fa_packet);
                          if (status != PacketManager::SerialStatus::Ok)
                          {
                              LOG_ERROR(LOG_TAG, "Failed sending fa packet (status %d)", (int)status);
                              callback.OnResult(ToAuthStatus(status), nullptr);
                              return ToStatus(status);
                          }

                          // number of retries in case of communication failure before giving up
                          const int max_retries = 3;
                          int retry_counter = 0;
                          while (true)
                          {
                              status = _session.RecvFaPacket(fa_packet);
                              if (status != PacketManager::SerialStatus::Ok)
                              {
                                  LOG_ERROR(LOG_TAG, "Failed receiving fa packet (status %d)", (int)status);
                                  callback.OnHint(ToAuthStatus(status));
                                  if (++retry_counter <= max_retries)
                                  {
                                      LOG_WARNING(LOG_TAG, "Timeout waiting for packet (try #%d)", retry_counter);
                                      continue;
                                  }
                                  return ToStatus(status);
                              }
                              // received successfully packet
                              retry_counter = 0;
                              auto msg_id = fa_packet.id;
                              auto fa_status = fa_packet.GetStatusCode();

                              switch (msg_id)
                              {
                                  // end of transaction
                              case (PacketManager::MsgId::Reply):
                                  return Status::Ok;

                              case (PacketManager::MsgId::Result): {
                                  callback.OnResult(AuthenticateStatus(fa_status), fa_packet.GetUserId());
                                  break;
                              }

                              case (PacketManager::MsgId::Hint):
                                  callback.OnHint(AuthenticateStatus(fa_status));
                                  break;

                              default:
                                  LOG_ERROR(LOG_TAG, "Got unexpected msg id in response: %d", (int)msg_id);
                                  callback.OnResult(AuthenticateStatus::Failure, nullptr);
                                  return Status::Error;
                              }
                          }
                      }
                      catch (std::exception& ex)
                      {
                          LOG_EXCEPTION(LOG_TAG, ex);
                          callback.OnResult(AuthenticateStatus::Failure, nullptr);
                          return Status::Error;
                      }
                      catch (...)
                      {
                          LOG_ERROR(LOG_TAG, "Unknown exception");
                          callback.OnResult(AuthenticateStatus::Failure, nullptr);
                          return Status::Error;
                      }
                  }

                  Status FaceAuthenticatorImpl::Cancel()
                  {
                      // Send cancel packet.
                      // Cancel can be sent without session to be able to cancel any previous open sessions.
                      try
                      {
                          PacketManager::FaPacket cancel_packet{ PacketManager::MsgId::Cancel };
                          PacketManager::PacketSender sender{ _serial.get() };
                          auto serial_status = sender.SendWithBinary1(cancel_packet);
                          if (serial_status != PacketManager::SerialStatus::Ok)
                          {
                              LOG_ERROR(LOG_TAG, "Failed sending cancel packet");
                          }
                          return ToStatus(serial_status);
                      }
                      catch (std::exception& ex)
                      {
                          LOG_EXCEPTION(LOG_TAG, ex);
                          return Status::Error;
                      }
                      catch (...)
                      {
                          LOG_ERROR(LOG_TAG, "Unknown exception");
                          return Status::Error;
                      }
                  } // namespace RealSenseID

                  Status FaceAuthenticatorImpl::RemoveUser(const char* user_id)
                  {
                      try
                      {
                          if (!ValidateUserId(user_id))
                          {
                              return Status::Error;
                          }
                          auto status = _session.Start(_serial.get());
                          if (status != PacketManager::SerialStatus::Ok)
                          {
                              LOG_ERROR(LOG_TAG, "Session start failed with status %d", static_cast<int>(status));
                              return ToStatus(status);
                          }

                          PacketManager::FaPacket fa_packet{ PacketManager::MsgId::RemoveUser, user_id, 0 };
                          status = _session.SendPacket(fa_packet);
                          if (status != PacketManager::SerialStatus::Ok)
                          {
                              LOG_ERROR(LOG_TAG, "Failed sending fa packet (status %d)", (int)status);
                              return ToStatus(status);
                          }

                          status = _session.RecvFaPacket(fa_packet);
                          if (status != PacketManager::SerialStatus::Ok)
                          {
                              LOG_ERROR(LOG_TAG, "Failed receiving fa packet (status %d)", (int)status);
                              return ToStatus(status);
                          }

                          return Status(fa_packet.GetStatusCode());
                      }
                      catch (std::exception& ex)
                      {
                          LOG_EXCEPTION(LOG_TAG, ex);
                          return Status::Error;
                      }
                      catch (...)
                      {
                          LOG_ERROR(LOG_TAG, "Unknown exception");
                          return Status::Error;
                      }
                  }

                  Status FaceAuthenticatorImpl::RemoveAllUsers()
                  {
                      try
                      {
                          auto status = _session.Start(_serial.get());
                          if (status != PacketManager::SerialStatus::Ok)
                          {
                              LOG_ERROR(LOG_TAG, "Session start failed with status %d", static_cast<int>(status));
                              return ToStatus(status);
                          }

                          PacketManager::FaPacket fa_packet{ PacketManager::MsgId::RemoveAllUsers };
                          status = _session.SendPacket(fa_packet);
                          if (status != PacketManager::SerialStatus::Ok)
                          {
                              LOG_ERROR(LOG_TAG, "Failed sending fa packet (status %d)", (int)status);
                              return ToStatus(status);
                          }

                          status = _session.RecvFaPacket(fa_packet);
                          if (status != PacketManager::SerialStatus::Ok)
                          {
                              LOG_ERROR(LOG_TAG, "Failed receiving fa packet (status %d)", (int)status);
                              return ToStatus(status);
                          }

                          return Status(fa_packet.GetStatusCode());
                      }
                      catch (std::exception& ex)
                      {
                          LOG_EXCEPTION(LOG_TAG, ex);
                          return Status::Error;
                      }
                      catch (...)
                      {
                          LOG_ERROR(LOG_TAG, "Unknown exception");
                          return Status::Error;
                      }
                  }

                  Status FaceAuthenticatorImpl::Standby()
                  {
                      try
                      {
                          auto status = _session.Start(_serial.get());
                          if (status != PacketManager::SerialStatus::Ok)
                          {
                              LOG_ERROR(LOG_TAG, "Session start failed with status %d", static_cast<int>(status));
                              return ToStatus(status);
                          }

                          PacketManager::DataPacket packet{ PacketManager::MsgId::StandBy };
                          status = _session.SendPacket(packet);
                          if (status != PacketManager::SerialStatus::Ok)
                          {
                              LOG_ERROR(LOG_TAG, "Failed sending fa packet (status %d)", static_cast<int>(status));
                              return ToStatus(status);
                          }

                          PacketManager::FaPacket fa_packet{ PacketManager::MsgId::Reply };
                          status = _session.RecvFaPacket(fa_packet);
                          if (status != PacketManager::SerialStatus::Ok)
                          {
                              LOG_ERROR(LOG_TAG, "Failed receiving fa packet (status %d)", static_cast<int>(status));
                              return ToStatus(status);
                          }
                          
                          auto status_code = fa_packet.GetStatusCode();
                          return static_cast<Status>(status_code);
                      }
                      catch (std::exception& ex)
                      {
                          LOG_EXCEPTION(LOG_TAG, ex);
                          return Status::Error;
                      }
                      catch (...)
                      {
                          LOG_ERROR(LOG_TAG, "Unknown exception");
                          return Status::Error;
                      }
                  }
                  Status FaceAuthenticatorImpl::QueryUserIds(char** user_ids, unsigned int& number_of_users)
                  {
                      unsigned int retrieved_user_count = 0;
                      constexpr unsigned int chunk_size = 5;

                      if (user_ids == nullptr || number_of_users == 0)
                      {
                          LOG_ERROR(LOG_TAG, "QueryUserIds: Got invalid params (nullptr or zero)");
                          number_of_users = 0;
                          return Status::Error;
                      }
                      
                      try
                      {
                          unsigned int arrived_users = 0;

                          for (unsigned int i = 0; i < number_of_users && retrieved_user_count < number_of_users; i += arrived_users)
                          {
                              LOG_DEBUG(LOG_TAG, "Get userids.  So far:%u", i);
                              auto status = _session.Start(_serial.get());
                              if (status != PacketManager::SerialStatus::Ok)
                              {
                                  LOG_ERROR(LOG_TAG, "Session start failed with status %d", static_cast<int>(status));
                                  number_of_users = 0;
                                  return ToStatus(status);
                              }
                              // retrieve next NUMBER_OF_USERS_TO_QUERY users
                              unsigned int settings[2];
                              settings[0] = retrieved_user_count;
                              settings[1] = chunk_size;

                              PacketManager::DataPacket query_users_packet{ PacketManager::MsgId::GetUserIds, (char*)settings,
                                                                            sizeof(settings) };
                              status = _session.SendPacket(query_users_packet);
                              if (status != PacketManager::SerialStatus::Ok)
                              {
                                  LOG_ERROR(LOG_TAG, "Failed sending data packet (status %d)", static_cast<int>(status));
                                  number_of_users = 0;
                                  return ToStatus(status);
                              }

                              PacketManager::DataPacket reply_packet{ PacketManager::MsgId::Versioning };
                              status = _session.RecvDataPacket(reply_packet);
                              if (status != PacketManager::SerialStatus::Ok)
                              {
                                  LOG_ERROR(LOG_TAG, "Failed receiving data packet (status %d)", static_cast<int>(status));
                                  number_of_users = 0;
                                  return ToStatus(status);
                              }

                              if (reply_packet.id != PacketManager::MsgId::GetUserIds)
                              {
                                  LOG_ERROR(LOG_TAG, "Unexpected msg id in reply (%c)", reply_packet.id);
                                  number_of_users = 0;
                                  return Status::Error;
                              }

                              // get number of users from the response
                              const char* data = reply_packet.Data().data;
                              ::memcpy(&arrived_users, data, sizeof(unsigned int));
                              if (arrived_users == 0)
                              {
                                  break;
                              }

                              // extract user ids from the returned chunk. each user id is zero delimited c string.
                              for (size_t j = 0, cur_pos = sizeof(unsigned int); j < arrived_users; j++)
                              {
                                  if (retrieved_user_count >= number_of_users)
                                  {
                                      break;
                                  }
                                  char* target = user_ids[retrieved_user_count];
                                  ::strncpy(target, &data[cur_pos], PacketManager::MaxUserIdSize);
                                  target[PacketManager::MaxUserIdSize] = '\0';
                                  cur_pos += ::strlen(target) + 1;
                                  retrieved_user_count++;
                              }
                          }

                          assert(retrieved_user_count <= number_of_users);
                          number_of_users = retrieved_user_count;

                          return Status::Ok;
                      }
                      catch (std::exception& ex)
                      {
                          LOG_EXCEPTION(LOG_TAG, ex);
                          number_of_users = 0;
                          return Status::Error;
                      }
                      catch (...)
                      {
                          LOG_ERROR(LOG_TAG, "Unknown exception");
                          number_of_users = 0;
                          return Status::Error;
                      }
                  }


                  Status FaceAuthenticatorImpl::QueryNumberOfUsers(unsigned int& number_of_users)
                  {
                      try
                      {
                          // switch to binary mode
                          auto status = _session.Start(_serial.get());
                          if (status != PacketManager::SerialStatus::Ok)
                          {
                              LOG_ERROR(LOG_TAG, "Session start failed with status %d", static_cast<int>(status));
                              number_of_users = 0;
                              return ToStatus(status);
                          }
                          PacketManager::DataPacket get_nusers_packet{ PacketManager::MsgId::GetNumberOfUsers };
                          status = _session.SendPacket(get_nusers_packet);
                          if (status != PacketManager::SerialStatus::Ok)
                          {
                              LOG_ERROR(LOG_TAG, "Failed sending data packet (status %d)", (int)status);
                              number_of_users = 0;
                              return ToStatus(status);
                          }

                          status = _session.RecvDataPacket(get_nusers_packet);
                          if (status != PacketManager::SerialStatus::Ok)
                          {
                              LOG_ERROR(LOG_TAG, "Failed receiving data packet (status %d)", (int)status);
                              number_of_users = 0;
                              return ToStatus(status);
                          }

                          if (get_nusers_packet.id != PacketManager::MsgId::GetNumberOfUsers)
                          {
                              LOG_ERROR(LOG_TAG, "Unexpected msg id in reply (%c)", get_nusers_packet.id);
                              number_of_users = 0;
                              return Status::Error;
                          }

                          ::memcpy(&number_of_users, &get_nusers_packet.payload.message.data_msg.data[0], sizeof(unsigned int));

                          return Status::Ok;
                      }
                      catch (std::exception& ex)
                      {
                          LOG_EXCEPTION(LOG_TAG, ex);
                          number_of_users = 0;
                          return Status::Error;
                      }
                      catch (...)
                      {
                          LOG_ERROR(LOG_TAG, "Unknown exception");
                          number_of_users = 0;
                          return Status::Error;
                      }
                  }
} // namespace RealSenseID

#ifdef _WIN32
#pragma warning(pop)
#endif
