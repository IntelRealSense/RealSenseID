"""
License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.
"""
import rsid_py

PORT='COM8'

if __name__ == '__main__':    
    with rsid_py.FaceAuthenticator(PORT) as f:
        #display list of enrolled users
        users = f.query_user_ids()
        print('Users: ', users)

        #delete all users
        if input("Delete all users [y/n]?") == 'y':
            f.remove_all_users()
        
    
    