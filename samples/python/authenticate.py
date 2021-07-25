"""
License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.
"""

import rsid_py

PORT='COM9'

def on_result(result, user_id):
    print('on_result', result)    
    if result == rsid_py.AuthenticateStatus.Success:
        print('Authenticated user:', user_id)

def on_faces(faces, timestamp):    
    print(f'detected {len(faces)} face(s)')
    for f in faces:
        print(f'\tface {f.x},{f.y} {f.w}x{f.h}')    

if __name__ == '__main__':
    with rsid_py.FaceAuthenticator(PORT) as f:
        f.authenticate(on_faces=on_faces, on_result=on_result)
    