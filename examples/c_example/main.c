// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "rsid_c/rsid_client.h"
#include "rsid_signature_example.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4996) // disable scanf msvc warning
#endif                          // _WIN32

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

void enroll_example(rsid_authenticator* authenticator, const char* user_id)
{
    printf("Enrolling user \"%s\"\n", user_id);
    rsid_enroll_args enroll_args;
    enroll_args.user_id = user_id; // user id. null terminated string of ascii chars (max 16 chars).
    enroll_args.status_clbk = my_enroll_status_clbk;
    enroll_args.hint_clbk = my_enroll_hint_clbk;
    enroll_args.progress_clbk = my_enroll_progress_clbk;
    enroll_args.ctx = NULL; /* user defined context struct. set to null if not needed. */

    rsid_enroll_status enroll_status = rsid_enroll(authenticator, &enroll_args);
    printf("Enroll result: %d (%s)\n", enroll_status, rsid_enroll_status_str(enroll_status));
}


/*******************************************************************************
 Authentication example.

 User defined callbacks are called during the authentication process
 Upon success (RSID_Auth_Success) the user_id param points to the user id.
 *******************************************************************************/
void my_auth_status_clbk(rsid_auth_status status, const char* user_id, void* ctx)
{
    printf("Autentication status: %d (%s)\n", status, rsid_auth_status_str(status));

    if (status == RSID_Auth_Success)
    {
        printf("Authenticion Success. User id: %s\n", user_id);
    }
}

void my_auth_hint_clbk(rsid_auth_status hint, void* ctx)
{
    printf("Authentication hint: %d (%s)\n", hint, rsid_auth_status_str(hint));
}

void authenticate_example(rsid_authenticator* authenticator)
{    
    printf("\nAuthenticating user..\n");
    rsid_auth_args auth_args;
    auth_args.result_clbk = my_auth_status_clbk;
    auth_args.hint_clbk = my_auth_hint_clbk;
    auth_args.ctx = NULL; /* user defined context struct. set to null if not needed. */

    rsid_auth_status auth_status = rsid_authenticate(authenticator, &auth_args);
    printf("Authenticate result: %d (%s)\n", auth_status, rsid_auth_status_str(auth_status));
}


void print_usage()
{
    printf("Usage: rsid_cpp_example <port> <usb/uart>\n");
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

void print_menu()
{
    printf("Enter a command:\n");
    printf("  'a' to authenticate.\n  'e' to enroll.\n  'd' to delete all users.\n  'q' to quit.\n");
}


void handle_input(char ch, rsid_authenticator* authenticator)
{
    switch (ch)
    {
    case 'e': {
        char user_id[17];
        printf("User id to enroll: ");
        if(scanf("%16s", user_id) != EOF)
            enroll_example(authenticator, user_id);
        break;
    }
    
    case 'a':
        authenticate_example(authenticator);
        break;

    case 'd':
        printf("Removing all users..\n");
        rsid_remove_all_users(authenticator);
        break;    
    }
}

int main(int argc, char* argv[])
{
    rsid_serial_config config = get_serial_config(argc, argv);

    printf("Creating authenticator..\n");    

    rsid_signature_clbk* signature_clbk = rsid_create_example_sig_clbk();
    rsid_authenticator* authenticator = rsid_create_authenticator(signature_clbk);
    if (!authenticator)
    {
        printf("Failed creating authenticator\n");
        return 1;
    }

    rsid_serial_status status = rsid_connect(authenticator, &config);
    if (status != RSID_Serial_Ok)
    {
        printf("Failed connecting: %s\n", rsid_serial_status_str(status));
        return 1;
    }
    char ch = 0;
    while (1)
    {
        print_menu();
        ch = getchar();
        if (ch == EOF || ch == 'q')
            break;
        handle_input(ch, authenticator);
        ch = getchar(); //consume newline        
    }    

    printf("\nClosing authenticator..\n");
    rsid_destroy_authenticator(authenticator);        
    rsid_destroy_example_sig_clbk(signature_clbk);
    return 0;
}


#ifdef _WIN32
#pragma warning(pop)
#endif // _WIN32