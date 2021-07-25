import os
import queue
import sys
import threading
import traceback
import time

try :
    import numpy as np
except ImportError:
    print('Failed importing numpy. Please install it (pip install numpy).')
    exit(0)

try:
    import cv2
except ImportError:
    print('Failed importing cv2. Please install it (pip install opencv-python).')
    exit(0)


import rsid_py

# globals
PORT = 'COM9'
WINDOW_NAME = 'RealSenseID'
window_init = False
status_msg = ''
# array of (faces, success, user_name)
detected_faces = []

print('Version: ' + rsid_py.__version__)

cmd_q = queue.Queue()


def show_menu(delay=2.5):
    def show_menu_():
        global status_msg
        global detected_faces
        time.sleep(delay)
        status_msg = "'a'=auth   'e'=enroll   'd'=delete"
        detected_faces = []

    threading.Thread(target=show_menu_, daemon=True).start()


def on_result(result, user_id=None):
    global status_msg
    global detected_faces
    
    success = result == rsid_py.AuthenticateStatus.Success
    status_msg = f'Success "{user_id}"' if success else str(result)
    
    #find next face without a status
    for f in detected_faces:
        if not 'success' in f:
            f['success'] = success
            f['user_id'] = user_id
            break

    


def on_progress(p):
    global status_msg
    status_msg = f'on_progress {p}'


def on_hint(h):
    global status_msg
    status_msg = f'{h}'


def on_faces(faces, timestamp):
    global status_msg
    global detected_faces
    status_msg = f'detected {len(faces)} face(s)'
    detected_faces = [{'face': f} for f in faces]

def auth_example():
    global status_msg
    with rsid_py.FaceAuthenticator(PORT) as f:
        status_msg = "Authenticating.."
        f.authenticate(on_hint=on_hint, on_result=on_result, on_faces=on_faces)


def enroll_example(user_id=f'user_{int(time.time() / 1000)}'):
    global status_msg
    with rsid_py.FaceAuthenticator(PORT) as f:
        status_msg = "Enroll.."
        f.enroll(on_hint=on_hint, on_progress=on_progress, on_result=on_result, on_faces=on_faces, user_id=user_id)


def remove_all_users():
    global status_msg
    with rsid_py.FaceAuthenticator(PORT) as f:
        status_msg = "Remove.."
        f.remove_all_users()
        status_msg = 'Remove Success'


def query_users():
    with rsid_py.FaceAuthenticator(PORT) as f:
        return f.query_user_ids()


def exit_app():
    global status_msg
    status_msg = 'Bye.. :)'
    time.sleep(0.4)
    os._exit(0)

cmd_exec = {'a': auth_example,
            'e': enroll_example,
            'd': remove_all_users,
            'q': exit_app}


def input_loop():
    show_menu(0)
    while True:
        detected_faces = []
        keycode = cmd_q.get()
        # exit on esc
        if keycode == 27: exit_app()
        cmd_exec.get(chr(keycode), lambda: None)()
        show_menu()


#########################
## Preview
#########################
def color_from_msg(msg):
    if 'Success' in msg:
        return (0x3c, 0xff, 0x3c)
    if 'Forbidden' in msg or 'Fail' in status_msg or 'NoFace' in status_msg:
        return (0x3c, 0x3c, 255)
    return (0xcc, 0xcc, 0xcc)


def show_face(face, image):
    # scale rets from 1080p
    f = face['face']

    scale_x = image.shape[1] / 1080.0
    scale_y = image.shape[0] / 1920.0
    x = int(f.x * scale_x)
    y = int(f.y * scale_y)
    w = int(f.w * scale_y)
    h = int(f.h * scale_y)

    start_point = (x, y)
    end_point = (x + w, y + h)
    success = face.get('success')
    if success is None:
        color = (0x33,0xcc, 0xcc) # yellow
    else:
        color = (0x11, 0xcc, 0x11) if success else (0x11, 0x11, 0xcc)
    thickness = 2
    cv2.rectangle(image, start_point, end_point, color, thickness)


    # show user id


def on_image(image):
    try:
        global window_init
        if not window_init:
            cv2.namedWindow(WINDOW_NAME, cv2.WINDOW_NORMAL)
            cv2.resizeWindow(WINDOW_NAME, (int(720 / 1.6), int(1280 / 1.6)))
            window_init = True
        buffer = memoryview(image.get_buffer())
        arr = np.asarray(buffer, dtype=np.uint8)
        array2d = arr.reshape((image.height, image.width, -1))
        img_rgb = cv2.cvtColor(array2d, cv2.COLOR_BGR2RGB)

        # show faces
        for f in detected_faces:
            show_face(f, img_rgb)

        img_rgb = cv2.flip(img_rgb, 1)

        color = color_from_msg(status_msg)
        img_rgb = show_status(status_msg, image=img_rgb, color=color)

        cv2.imshow(WINDOW_NAME, img_rgb)
        key = cv2.waitKey(20)
        if key != -1:
            cmd_q.put(key)
    except Exception:
        print("Exception")
        print("-" * 60)
        traceback.print_exc(file=sys.stdout)
        print("-" * 60)
        os._exit(1)


def show_status(msg, image, color):
    font = cv2.FONT_HERSHEY_SIMPLEX
    font_scale = 1.4
    thickness = 3
    padding = 20

    (msg_w, msg_h), _ = cv2.getTextSize(msg, font, fontScale=font_scale, thickness=thickness)
    image_h, image_w = image.shape[0], image.shape[1]
    rect_x = 0
    rect_y = image_h - msg_h - padding * 2

    start_point = (rect_x, rect_y)
    end_point = (image_w, image_h)

    bg_color = (0x33, 0x33, 0x33)
    image = cv2.rectangle(image, start_point, end_point, bg_color, -thickness)
    # align to center
    text_x = int((image_w - msg_w) / 2)
    text_y = rect_y + msg_h + padding
    msg = msg.replace('Status.', ' ')
    return cv2.putText(image, msg, (text_x, text_y), font, font_scale, color, thickness, cv2.LINE_AA)


def preview_loop():
    preview_cfg = rsid_py.PreviewConfig()
    preview_cfg.camera_number = -1 # -1 means auto detect
    p = rsid_py.Preview(preview_cfg)
    p.start(on_image)
    while True: time.sleep(1)


def main():
    preview_thread = threading.Thread(target=preview_loop, daemon=True)
    preview_thread.start()
    input_loop()


if __name__ == '__main__':
    main()
