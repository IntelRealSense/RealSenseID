"""
License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.
"""
import rsid_py

PORT='COM9'

def on_result(result):
    print('on_result', result)    

def on_progress(p):    
    print(f'on_progress {p}')

def on_hint(h):    
    print(f'on_hint {h}')

def on_faces(faces, timestamp):    
    print(f'detected {len(faces)} face(s)')
    for f in faces:
        print(f'\tface {f.x},{f.y} {f.w}x{f.h}')    


if __name__ == '__main__':
    with rsid_py.FaceAuthenticator(PORT) as f:
        user_id = input("User id to enroll: ")
        f.enroll(user_id=user_id, on_hint=on_hint, on_progress=on_progress, on_faces=on_faces, on_result=on_result)

        #display list of enrolled users
        users = f.query_user_ids()
        print('Users: ', users)
    
    