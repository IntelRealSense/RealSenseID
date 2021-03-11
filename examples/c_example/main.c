// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "rsid_c/rsid_client.h"
#ifdef RSID_SECURE
#include "rsid_signature_example.h"
static rsid_signature_clbk* s_signer = NULL;
#endif // RSID_SECURE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// Create FaceAuthenticator (after successfully connecting it to the device).
// If failed to connect, exit(1)
rsid_authenticator* create_auth(rsid_serial_config* serial_config)
{
#ifdef RSID_SECURE
    rsid_authenticator* authenticator = rsid_create_authenticator(s_signer);
#else
    rsid_authenticator* authenticator = rsid_create_authenticator();
#endif

    if (!authenticator)
    {
        printf("Failed creating authenticator\n");
        exit(1);
    }

    rsid_status status = rsid_connect(authenticator, serial_config);
    if (status != RSID_Ok)
    {
        printf("Failed connecting: %s\n", rsid_status_str(status));
        exit(1);
    }
    return authenticator;
}

/*******************************************************************************
 Enrollment example
 User defined callbacks are called during the enrollment process
 *******************************************************************************/
void my_enroll_status_clbk(rsid_enroll_status status, void* ctx)
{
    printf("Enroll status: %d (%s)\n", status, rsid_enroll_status_str(status));
    if (status == RSID_Enroll_Success)
    {
        printf("Enroll success\n");
    }
}

void my_enroll_hint_clbk(rsid_enroll_status hint, void* ctx)
{
    printf("Enroll hint: %d (%s)\n", hint, rsid_enroll_status_str(hint));
}

void my_enroll_progress_clbk(rsid_face_pose pose, void* ctx)
{
    printf("Enroll face pose callback: %d (%s)\n", pose, rsid_face_pose_str(pose));
}

void enroll_example(rsid_serial_config* serial_config, const char* user_id)
{
    rsid_authenticator* authenticator = create_auth(serial_config);
    printf("Enrolling user \"%s\"\n", user_id);
    rsid_enroll_args enroll_args;
    enroll_args.user_id = user_id; // user id. null terminated string of ascii chars (max 16 chars).
    enroll_args.status_clbk = my_enroll_status_clbk;
    enroll_args.hint_clbk = my_enroll_hint_clbk;
    enroll_args.progress_clbk = my_enroll_progress_clbk;
    enroll_args.ctx = NULL; /* user defined context struct. set to null if not needed. */

    rsid_status status = rsid_enroll(authenticator, &enroll_args);
    if (status != RSID_Ok)
    {
        printf("Error status: %d (%s)\n", status, rsid_status_str(status));
    }

    rsid_destroy_authenticator(authenticator);
}

/*******************************************************************************
 Authentication example.

 User defined callbacks are called during the authentication process
 Upon success (RSID_Auth_Success) the user_id param points to the user id.
 *******************************************************************************/
void my_auth_status_clbk(rsid_auth_status status, const char* user_id, void* ctx)
{
    printf("Authentication status: %d (%s)\n", status, rsid_auth_status_str(status));

    if (status == RSID_Auth_Success)
    {
        printf("Authentication Success. User id: %s\n", user_id);
    }
}

void my_auth_hint_clbk(rsid_auth_status hint, void* ctx)
{
    printf("Authentication hint: %d (%s)\n", hint, rsid_auth_status_str(hint));
}

void authenticate_example(rsid_serial_config* serial_config)
{
    rsid_authenticator* authenticator = create_auth(serial_config);
    printf("\nAuthenticating user..\n");
    rsid_auth_args auth_args;
    auth_args.result_clbk = my_auth_status_clbk;
    auth_args.hint_clbk = my_auth_hint_clbk;
    auth_args.ctx = NULL; /* user defined context struct. set to null if not needed. */

    rsid_status status = rsid_authenticate(authenticator, &auth_args);
    if (status != RSID_Ok)
    {
        printf("Error status: %d (%s)\n", status, rsid_status_str(status));
    }

    rsid_destroy_authenticator(authenticator);
}

void remove_all_users_example(rsid_serial_config* serial_config)
{
    rsid_authenticator* authenticator = create_auth(serial_config);
    printf("\nRemove all users..\n");
    rsid_status status = rsid_remove_all_users(authenticator);
    printf("Result: %d (%s)\n", (int)status, rsid_status_str(status));
    rsid_destroy_authenticator(authenticator);
}

#ifdef RSID_SECURE
/*******************************************************************************
Pairing example -exchange with the device public keys.
*******************************************************************************/
void pairing_example(rsid_serial_config* serial_config)
{
    rsid_authenticator* authenticator = create_auth(serial_config);
    rsid_pairing_args args = {0};
    const unsigned char* host_pubkey = rsid_get_host_pubkey_example(s_signer);
    memcpy(args.host_pubkey, host_pubkey, sizeof(args.host_pubkey));
    rsid_status status = rsid_pair(authenticator, &args);
    if (status != RSID_Ok)
    {
        printf("Error status: %d (%s)\n", status, rsid_status_str(status));
        rsid_destroy_authenticator(authenticator);
        return;
    }

    // now store the device public key for future communication
    rsid_update_device_pubkey_example(s_signer, (unsigned char*)args.device_pubkey_result);
    rsid_destroy_authenticator(authenticator);
}

/*******************************************************************************
Unpairing example - Disable mutual authentication.
*******************************************************************************/
void unpairing_example(rsid_serial_config* serial_config)
{
    rsid_authenticator* authenticator = create_auth(serial_config);
    rsid_status status = rsid_unpair(authenticator);
    if (status != RSID_Ok)
    {
        printf("Error status: %d (%s)\n", status, rsid_status_str(status));
    }
    rsid_destroy_authenticator(authenticator);
}
#endif // RSID_SECURE

void query_auth_settings_example(rsid_serial_config* serial_config)
{
    rsid_authenticator* authenticator = create_auth(serial_config);
    printf("\nQuery auth settings..\n");
    rsid_auth_config auth_config = {0};
    rsid_status status = rsid_query_auth_settings(authenticator, &auth_config);
    if (status != RSID_Ok)
    {
        printf("Error status: %d (%s)\n", status, rsid_status_str(status));
        rsid_destroy_authenticator(authenticator);
        return;
    }
    printf("\nAuth settings:\n===============\n");
    printf("Camera Rotation: %s\n", rsid_auth_settings_rotation(auth_config.camera_rotation));
    printf("Security Level:  %s\n\n", rsid_auth_settings_level(auth_config.security_level));
    rsid_destroy_authenticator(authenticator);
}

void query_userids_example(rsid_serial_config* serial_config)
{
    rsid_authenticator* authenticator = create_auth(serial_config);

    unsigned int number_of_users = 0;
    rsid_status status = rsid_query_number_of_users(authenticator, &number_of_users);
    if (status != RSID_Ok)
    {
        printf("Got error:  %d\n\n", (int)status);
        rsid_destroy_authenticator(authenticator);
        return;
    }

    if (number_of_users == 0)
    {
        printf("No users found\n\n");
        rsid_destroy_authenticator(authenticator);
        return;
    }
// allocate needed array of user ids
#define RSID_USERID_SIZE 16
    char** user_ids = malloc(number_of_users * sizeof(char*));
    if (user_ids == NULL)
    {
        printf("Alloc failed\n");
        rsid_destroy_authenticator(authenticator);
        return;
    }
    for (unsigned i = 0; i < number_of_users; i++)
    {
        user_ids[i] = malloc(RSID_USERID_SIZE);
    }

    unsigned int nusers_in_out = number_of_users;
    status = rsid_query_user_ids(authenticator, user_ids, &nusers_in_out);
    if (status != RSID_Ok)
    {
        printf("Got error:  %d\n\n", (int)status);
        goto clean_exit;
    }

    printf("\n%u Users:\n==========\n", nusers_in_out);
    for (unsigned int i = 0; i < nusers_in_out; i++)
    {
        printf("%u.  %s \n", i + 1, user_ids[i]);
    }
    printf("\n");

clean_exit:

    // free allocated memory
    for (unsigned i = 0; i < number_of_users; i++)
    {
        free(user_ids[i]);
    }
    free(user_ids);
    // release authenticator
    rsid_destroy_authenticator(authenticator);
}

void query_nusers_example(rsid_serial_config* serial_config)
{
    rsid_authenticator* authenticator = create_auth(serial_config);

    unsigned int nusers = 0;
    rsid_status status = rsid_query_number_of_users(authenticator, &nusers);
    if (status == RSID_Ok)
    {
        printf("Number for users: %u\n\n", nusers);
    }
    else
    {
        printf("Got error status %d\n\n", (int)status);
    }
    rsid_destroy_authenticator(authenticator);
}

void additional_information_example(rsid_serial_config* serial_config)
{
    rsid_device_controller* device_controller = rsid_create_device_controller();
    rsid_status status = rsid_connect_controller(device_controller, serial_config);
    if (status != RSID_Ok)
    {
        printf("Failed connecting: %s\n", rsid_status_str(status));
        rsid_destroy_device_controller(device_controller);
        return;
    }

    const char* host_version = rsid_version();

    char fw_version[250];
    status = rsid_query_firmware_version(device_controller, fw_version, sizeof(fw_version));
    if (status != RSID_Ok)
    {
        printf("Error trying to query firmware version. Status: %d (%s)\n", status, rsid_status_str(status));
        rsid_destroy_device_controller(device_controller);
        return;
    }

    char serial_number[30];
    status = rsid_query_serial_number(device_controller, serial_number, sizeof(serial_number));
    if (status != RSID_Ok)
    {
        printf("Error trying to query serial number. Status: %d (%s)\n", status, rsid_status_str(status));
        rsid_destroy_device_controller(device_controller);
        return;
    }

    printf("\n");
    printf("Additional information:\n");
    printf(" * S/N: %s\n", serial_number);
    printf(" * Firmware: %s\n", fw_version);
    printf(" * Host: %s\n", host_version);
    printf("\n");

    rsid_destroy_device_controller(device_controller);
}

void ping_example(rsid_serial_config* serial_config, int iters)
{
    rsid_device_controller* device_controller = rsid_create_device_controller();
    if (device_controller == NULL)
    {
        printf("Failed creating device controller\n");
        exit(1);
    }

    rsid_status status = rsid_connect_controller(device_controller, serial_config);
    if (status != RSID_Ok)
    {
        printf("Failed connecting: %s\n", rsid_status_str(status));
        rsid_destroy_device_controller(device_controller);
        return;
    }

    for (int i = 0; i < iters; i++)
    {
        status = rsid_ping(device_controller);
        printf("Ping #%04d %s. \n\n", (i + 1), rsid_status_str(status));
        if (status != RSID_Ok)
        {
            printf("Ping error\n\n");
            break;
        }
    }

    rsid_destroy_device_controller(device_controller);
}

void print_usage()
{
    printf("Usage: rsid_c_example <port> <usb/uart>\n");
}

rsid_serial_config get_serial_config(int argc, char* argv[])
{
    if (argc < 3)
    {
        print_usage();
        exit(1);
    }

    rsid_serial_config config;
    config.port = argv[1];

    config.port = argv[1];
    const char* ser_type = argv[2];
    if (!strcmp(ser_type, "usb"))
    {
        config.serial_type = RSID_USB;
    }
    else if (!strcmp(ser_type, "uart"))
    {
        config.serial_type = RSID_UART;
    }
    else
    {
        print_usage();
        exit(1);
    }

    return config;
}

void print_menu_opt(const char* line)
{
    printf("  %s\n", line);
}

void print_menu()
{
    printf("please select an option:\n\n");
    print_menu_opt("'a' to authenticate.");
    print_menu_opt("'e' to enroll.");
    print_menu_opt("'d' to delete all users.");
#ifdef RSID_SECURE
    print_menu_opt("'p' to pair with the device (must be performed at least once).");
    print_menu_opt("'i' to unpair with the device (disables secure communication).");
#endif // RSID_SECURE
    print_menu_opt("'g' to query authentication settings.");
    print_menu_opt("'u' to query ids of users.");
    print_menu_opt("'n' to query number of users.");
    print_menu_opt("'v' to view additional information.");
    print_menu_opt("'x' to ping the device.");
    print_menu_opt("'q' to quit.");
    printf("\n");
    printf("> ");
}

void handle_input(char ch, rsid_serial_config* serial_config)
{
    switch (ch)
    {
    case 'e': {
        char user_id[17];
        printf("User id to enroll: ");
        if (scanf("%15s", user_id) != EOF)
            enroll_example(serial_config, user_id);
        break;
    }
    case 'a':
        authenticate_example(serial_config);
        break;
    case 'd':
        remove_all_users_example(serial_config);
        break;
#ifdef RSID_SECURE
    case 'p':
        pairing_example(serial_config);
        break;
    case 'i':
        unpairing_example(serial_config);
        break;
#endif // RSID_SECURE
    case 'g':
        query_auth_settings_example(serial_config);
        break;
    case 'u':
        query_userids_example(serial_config);
        break;
    case 'n':
        query_nusers_example(serial_config);
        break;
    case 'v':
        additional_information_example(serial_config);
        break;
    case 'x': {
        int iters = -1;
        do
        {
            printf("Interations:\n> ");
            int rv = scanf("%d", &iters);
            if (rv == EOF)
                return;
            if (rv == 0)
            {
                // invalid input. flush this line
                while (fgetc(stdin) != '\n')
                {
                }
                continue;
            }

        } while (iters < 0);
        ping_example(serial_config, iters);
        break;
    }
    }
}

int main(int argc, char* argv[])
{
    rsid_serial_config serial_config = get_serial_config(argc, argv);
#ifdef RSID_SECURE
    s_signer = rsid_create_example_sig_clbk();
#endif // RSID_SECURE

    int ch = 0;
    while (1)
    {
        print_menu();
        ch = getchar();
        if (ch == EOF || ch == (int)'q')
            break;
        handle_input(ch, &serial_config);
        ch = getchar(); // consume newline
    }

#ifdef RSID_SECURE
    rsid_destroy_example_sig_clbk(s_signer);
#endif // RSID_SECURE
    return 0;
}
