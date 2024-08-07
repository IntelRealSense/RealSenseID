#!/usr/bin/env python3

"""
License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2020-2024 Intel Corporation. All Rights Reserved.
"""

import argparse
import copy
import ctypes
import io
import os
import pathlib
import queue
import signal
import sys
import threading
import time
import traceback

import PIL

try:
    import numpy as np
except ImportError:
    print('Failed importing numpy. Please install it (pip install numpy).')
    print('  On Ubuntu, you may install the system wide package instead: sudo apt install python3-numpy')
    exit(1)

try:
    import tkinter as tk
    import tkinter.ttk as ttk
    from tkinter import messagebox, simpledialog
except ImportError as ex:
    print(f'Failed importing tkinter ({ex}).')
    print('  On Ubuntu, you also need to: sudo apt install python3-tk')
    print('  On Fedora, you also need to: sudo dnf install python3-tkinter')
    exit(1)

try:
    from PIL import Image, ImageDraw, ImageOps, ImageTk
except ImportError as ex:
    print(f'Failed importing PIL ({ex}). Please install Pillow *version 9.1.0 or newer* (pip install Pillow).')
    print(
        '  On Ubuntu, you may install the system wide package instead: sudo apt install python3-pil python3-pil.imagetk')
    exit(1)

import rsid_py

print('Version: ' + rsid_py.__version__)

# globals
WINDOW_NAME = 'RealSenseID'


class Controller(threading.Thread):
    status_msg: str
    detected_faces: list[dict]  # array of (faces, success, user_name)
    running: bool = True
    window_init: bool = False
    image_q: queue.Queue = queue.Queue()
    snapshot_q: queue.Queue = queue.Queue()

    def __init__(self, port: str, camera_index: int, dump_mode: rsid_py.DumpMode):
        super().__init__()
        self.preview = None
        self.status_msg = ''
        self.detected_faces = []
        self.port = port
        self.camera_index = camera_index
        self.dump_mode = dump_mode
        if self.dump_mode in [rsid_py.DumpMode.CroppedFace, rsid_py.DumpMode.FullFrame]:
            self.status_msg = '-- Dump Mode --' \
                if rsid_py.DumpMode.FullFrame == self.dump_mode else '-- Cropped Face --'
            (pathlib.Path('.') / 'dumps').mkdir(parents=True, exist_ok=True)
            if not (pathlib.Path('.') / 'dumps').exists():
                raise RuntimeError('Unable to create dumps directory.')

    def reset(self):
        self.status_msg = ''
        self.detected_faces = []

    def on_result(self, result, user_id=None):
        success = result == rsid_py.AuthenticateStatus.Success
        self.status_msg = f'Success "{user_id}"' if success else str(result)

        # find next face without a status
        for f in self.detected_faces:
            if 'success' not in f:
                f['success'] = success
                f['user_id'] = user_id
                break

    def on_progress(self, p: rsid_py.FacePose):
        self.status_msg = f'on_progress {p}'

    def on_hint(self, hint: rsid_py.AuthenticateStatus | rsid_py.EnrollStatus | None):
        self.status_msg = f'{hint}'

    def on_faces(self, faces: list[rsid_py.FaceRect], timestamp: int):
        self.status_msg = f'detected {len(faces)} face(s)'
        self.detected_faces = [{'face': f} for f in faces]

    def auth_example(self):
        with rsid_py.FaceAuthenticator(self.port) as f:
            self.status_msg = "Authenticating.."
            f.authenticate(on_hint=self.on_hint, on_result=self.on_result, on_faces=self.on_faces)

    def enroll_example(self, user_id=f'user_{int(time.time() / 1000)}'):
        with rsid_py.FaceAuthenticator(self.port) as f:
            self.status_msg = "Enroll.."
            f.enroll(on_hint=self.on_hint, on_progress=self.on_progress,
                     on_result=self.on_result, on_faces=self.on_faces, user_id=user_id)

    def remove_all_users(self):
        with rsid_py.FaceAuthenticator(self.port) as f:
            self.status_msg = "Remove.."
            f.remove_all_users()
            self.status_msg = 'Remove Success'

    def query_users(self):
        with rsid_py.FaceAuthenticator(self.port) as f:
            return f.query_user_ids()

    #########################
    # Preview
    #########################
    def on_image(self, image: rsid_py.Image):
        if not self.running:
            return
        try:
            buffer = memoryview(image.get_buffer())
            arr = np.asarray(buffer, dtype=np.uint8)
            array2d = arr.reshape((image.height, image.width, -1))
            self.image_q.put(array2d.copy())
        except Exception:
            print("Exception")
            print("-" * 60)
            traceback.print_exc(file=sys.stdout)
            print("-" * 60)

    def on_snapshot(self, image: rsid_py.Image):
        try:
            if self.dump_mode == rsid_py.DumpMode.FullFrame:
                buffer = copy.copy(bytearray(image.get_buffer()))
                dump_path = (pathlib.Path('.') / 'dumps' / f'timestamp-{image.metadata.timestamp}')
                dump_path.mkdir(parents=True, exist_ok=True)
                print(f"frame metadata: "
                      f"ts: {image.metadata.timestamp}, "
                      f"status: {image.metadata.status}, "
                      f"sensor_id: {image.metadata.sensor_id}, "
                      f"exposure: {image.metadata.exposure}, "
                      f"gain: {image.metadata.gain}, "
                      f"led: {image.metadata.led}")
                file_name = (f'{image.metadata.timestamp}-{image.metadata.status}-{image.metadata.sensor_id}-'
                             f'{image.metadata.exposure}-{image.metadata.gain}.w10')
                file_path = dump_path / file_name
                with open(file_path, 'wb') as fd:
                    fd.write(buffer)
                print(f'RAW File saved to: {file_path.absolute()}')
            elif self.dump_mode == rsid_py.DumpMode.CroppedFace:
                buffer = image.get_buffer()
                # https://pillow.readthedocs.io/en/stable/reference/Image.html#PIL.Image.frombytes
                # https://pillow.readthedocs.io/en/stable/handbook/writing-your-own-image-plugin.html#the-raw-decoder
                image = Image.frombytes('RGB', (image.width, image.height), buffer, 'raw',
                                        'RGB', 0, 1)
                self.snapshot_q.put(image)
        except Exception:
            print("Exception")
            print("-" * 60)
            traceback.print_exc(file=sys.stdout)
            print("-" * 60)

    def start_preview(self):
        preview_cfg = rsid_py.PreviewConfig()
        preview_cfg.camera_number = self.camera_index
        if self.dump_mode == rsid_py.DumpMode.FullFrame:
            preview_cfg.preview_mode = rsid_py.PreviewMode.RAW10_1080P  # In dump mode, we can use RAW10
        elif self.dump_mode in [rsid_py.DumpMode.CroppedFace, rsid_py.DumpMode.Disable]:
            preview_cfg.preview_mode = rsid_py.PreviewMode.MJPEG_1080P

        self.preview = rsid_py.Preview(preview_cfg)
        self.preview.start(preview_callback=self.on_image, snapshot_callback=self.on_snapshot)

    def run(self):
        self.start_preview()
        while self.running:
            time.sleep(0.1)
        self.preview.stop()
        self.preview = None
        print("Controller thread exited")

    def exit_thread(self):
        self.status_msg = 'Bye.. :)'
        self.running = False
        time.sleep(0.5)


class GUI(tk.Tk):
    def __init__(self, controller: Controller):
        super().__init__(className=WINDOW_NAME)
        self.scaled_image = None
        self.image = None
        self.snapshot_image = None
        self.controller = controller
        self.reset_handle = None
        self.video_update_handle = None
        self.resize_handle = None
        self.snapshot_handle = None

        self.title(WINDOW_NAME)
        max_w = int(720 / 1.5)
        max_h = int(1280 / 1.5) + 80
        self.geometry(f"{max_w}x{max_h}")
        self.minsize(int(max_w / 1.5), int(max_h / 2.5))
        self.maxsize(max_w, max_h)

        # Window bindings
        self.protocol("WM_DELETE_WINDOW", self.exit_app)
        self.bind('<Escape>', lambda e: self.exit_app())
        self.bind("<Configure>", self.resize)

        self.grid_columnconfigure((1, 0), weight=1)
        self.grid_rowconfigure((1, 0), weight=1)

        # Video frame
        self.video_frame = ttk.Frame(self)
        self.video_frame.grid(row=0, column=0, padx=(0, 0), pady=(0, 20), sticky="nsew", columnspan=2)
        self.video_frame.grid_rowconfigure((0, 1), weight=1)
        self.video_frame.grid_columnconfigure((0, 1), weight=1)
        self.canvas = tk.Canvas(self.video_frame, bg='black')
        self.canvas.grid(row=0, column=0, padx=0, pady=0, sticky="nsew", columnspan=1)
        self.canvas.configure(width=max_w, height=max_h)

        # Canvas
        self.reset_canvas = True
        self.canvas_image_id = None
        self.canvas_text_id = None
        self.canvas_text_bg_id = None
        self.canvas_snapshot_image_id = None

        # Button frame
        self.button_frame = ttk.Frame(self)
        self.button_frame.grid(row=1, column=0, padx=(5, 5), pady=(0, 5), sticky="nsew", columnspan=2)

        self.auth_button = ttk.Button(self.button_frame, text="Authenticate",
                                      command=self.authenticate)
        self.auth_button.grid(row=0, column=0, padx=(5, 5), pady=(5, 5), ipady=5, sticky="nsew")

        self.enroll_button = ttk.Button(self.button_frame, text="Enroll",
                                        command=self.enroll)
        self.enroll_button.grid(row=0, column=1, padx=(5, 5), pady=(5, 5), ipady=5, sticky="nsew")

        self.delete_button = ttk.Button(self.button_frame, text="Delete All",
                                        command=self.remove_all_users)
        self.delete_button.grid(row=0, column=2, padx=(5, 5), pady=(5, 5), ipady=5, sticky="nsew")

        self.button_frame.grid_columnconfigure(0, weight=1)
        self.button_frame.grid_columnconfigure(1, weight=1)
        self.button_frame.grid_columnconfigure(2, weight=1)

        style = ttk.Style(self)
        if sys.platform.startswith('win'):
            style.theme_use('vista')
        else:
            style.theme_use('clam')

        self.bind("<Key>", self.key_event)
        self.after(50, self.update_video)
        self.after(200, self.update_app_icon)

    def update_app_icon(self):
        # Window Icon
        icon = Image.new("RGB", (50, 50))
        op = ImageDraw.Draw(icon)
        op.text((10, 0), "R", font_size=40, fill="white")
        self.icon = ImageTk.PhotoImage(icon)
        self.wm_iconphoto(False, self.icon)

    def key_event(self, event):
        cmd_exec = {'a': self.authenticate,
                    'e': self.enroll,
                    'd': self.remove_all_users,
                    'q': self.exit_app}
        cmd_exec.get(event.char, lambda: None)()

    def resize(self, event):
        if event.widget == self.canvas:
            self.canvas.configure(width=event.width, height=event.height)
            self.reset_canvas = True
            if self.resize_handle is not None:
                self.after_cancel(self.resize_handle)
            if self.resize_handle is None:
                self.resize_handle = self.after(100, self.canvas.update_idletasks)

    def reset_later(self):
        if self.reset_handle is not None:
            self.after_cancel(self.reset_handle)
        self.reset_handle = self.after(3 * 1000, self.controller_reset)

    def controller_reset(self):
        self.controller.reset()
        self.reset_canvas = True

    def authenticate(self):
        self.controller.reset()
        self.controller.auth_example()
        self.reset_later()

    def remove_all_users(self):
        self.controller.reset()
        result = messagebox.askyesno("Remove all users",
                                     "Are you sure you want to remove all users?",
                                     parent=self)
        if result:
            self.controller.remove_all_users()
            self.reset_later()

    def enroll(self):
        self.controller.reset()
        user_input = simpledialog.askstring(prompt="User ID:", title="Enter user id", parent=self)
        if user_input is not None:
            self.controller.enroll_example(user_input)
            self.reset_later()

    def clear_snapshot(self):
        self.canvas.itemconfig(self.canvas_snapshot_image_id, image=None)
        self.canvas.itemconfig(self.canvas_snapshot_image_id, state='hidden')
        self.snapshot_handle = None

    def update_video(self):
        self.update_idletasks()
        if not self.controller.image_q.empty() and self.controller.running:
            array2d = None
            while not self.controller.image_q.empty():
                array2d = self.controller.image_q.get()
            try:
                self.image = Image.fromarray(array2d, mode="RGB")
            except PIL.UnidentifiedImageError:
                print("Preview Error: UnidentifiedImageError")

        self.canvas.update_idletasks()
        canvas_h = self.canvas.winfo_reqheight()
        canvas_w = self.canvas.winfo_reqwidth()

        if self.image is not None:
            image = self.image.copy()
            # Render faces
            for f in self.controller.detected_faces:
                self.render_face_rect(f, image)

            scaled_image = ImageOps.contain(image, size=(canvas_w, canvas_h)).transpose(Image.Transpose.FLIP_LEFT_RIGHT)
            self.scaled_image = ImageTk.PhotoImage(image=scaled_image)

            if self.reset_canvas:
                self.canvas.delete("all")
                self.canvas_image_id = self.canvas.create_image(int(self.scaled_image.width() / 2),
                                                                int(self.scaled_image.height() / 2),
                                                                anchor=tk.CENTER, image=None)
                self.canvas_text_bg_id = self.canvas.create_rectangle(0, canvas_h - 50, canvas_w, canvas_h,
                                                                      fill='black', stipple='gray50')
                self.canvas_text_id = self.canvas.create_text(canvas_w / 2, canvas_h - 30, text='',
                                                              font='Helvetica 18 bold')
                self.canvas_snapshot_image_id = self.canvas.create_image(0, 0, anchor=tk.NW, image=None)
                self.canvas.itemconfig(self.canvas_snapshot_image_id, state='hidden')
                self.reset_canvas = False

            self.canvas.itemconfig(self.canvas_image_id, image=self.scaled_image)
            self.canvas.moveto(self.canvas_image_id, int((canvas_w - self.scaled_image.width()) / 2),
                               int((canvas_h - self.scaled_image.height()) / 2))

            # Render message
            msg = self.controller.status_msg.replace('Status.', ' ')
            if msg != '':
                color = self.color_from_msg(self.controller.status_msg)
                self.canvas.itemconfig(self.canvas_text_bg_id, state='normal')
                self.canvas.itemconfig(self.canvas_text_id, state='normal', text=msg, fill=color)
            else:
                self.canvas.itemconfig(self.canvas_text_bg_id, state='hidden')
                self.canvas.itemconfig(self.canvas_text_id, state='hidden')

            # Render snapshot
            new_snapshot = None
            while not self.controller.snapshot_q.empty():
                new_snapshot = self.controller.snapshot_q.get()

            if new_snapshot is not None:
                scaled_image = ImageOps.contain(new_snapshot, size=(int(canvas_w / 4), int(canvas_h / 4))).transpose(
                    Image.Transpose.FLIP_LEFT_RIGHT)

                self.snapshot_image = ImageTk.PhotoImage(image=scaled_image)

                if self.snapshot_handle is not None:
                    self.after_cancel(self.snapshot_handle)
                self.snapshot_handle = self.after(5000, self.clear_snapshot)
                self.canvas.itemconfig(self.canvas_snapshot_image_id, image=self.snapshot_image)
                self.canvas.itemconfig(self.canvas_snapshot_image_id, state='normal')

        elif self.image is None and self.controller.dump_mode:
            self.canvas.delete("all")
            self.canvas.create_text(canvas_w / 2, canvas_h / 2,
                                    text='Dump Mode', fill='white', font='Helvetica 20 bold')
            self.canvas.create_text(canvas_w / 2, (canvas_h / 2) + 30,
                                    text='Auth or Enroll to proceed', fill='white', font='Helvetica 14')

        self.update_idletasks()

        if self.video_update_handle is not None:
            self.after_cancel(self.video_update_handle)
        if self.controller.running:
            self.video_update_handle = self.after(15, self.update_video)

    @staticmethod
    def render_face_rect(face, image):
        img1 = ImageDraw.Draw(image)
        f = face['face']

        success = face.get('success')
        if success is None:
            color = 'yellow'
        else:
            color = 'green' if success else 'blue'

        shape = [(f.x, f.y), (f.x + f.w, f.y + f.h)]
        img1.rectangle(shape, width=8, outline=color)

    @staticmethod
    def color_from_msg(msg):
        if 'Success' in msg:
            return 'lime green'  # 0x3c, 0xff, 0x3c
        if 'Forbidden' in msg or 'Fail' in msg or 'NoFace' in msg:
            return 'RoyalBlue1'  # 0x3c, 0x3c, 255
        return 'gray80'  # 0xcc, 0xcc, 0xcc

    def exit_app(self):
        self.controller.exit_thread()
        self.quit()


def main():
    arg_parser = argparse.ArgumentParser(prog='viewer', add_help=False)
    options = arg_parser.add_argument_group('Options')
    options.add_argument('-h', '--help', action='help', default=argparse.SUPPRESS,
                         help='Show this help message and exit.')
    options.add_argument('-p', '--port', help='Device port. Will detect first device '
                                              'port if not specified.', type=str)
    options.add_argument('-c', '--camera', help='Camera number. -1 for autodetect.', type=int, default=-1)

    group = arg_parser.add_mutually_exclusive_group(required=False)
    group.add_argument('-d', '--dump', help='Dump mode.', action='store_true')
    group.add_argument('-r', '--crop', help='Cropped Face mode.', action='store_true')

    args = arg_parser.parse_args()
    port = None
    camera_index = args.camera

    if args.port is None:
        devices = rsid_py.discover_devices()
        if len(devices) == 0:
            print('Error: No rsid devices were found and no port was specified.')
            exit(1)
        port = devices[0]
    else:
        port = args.port

    print(f'Using self.port: {port}')
    print(f'Using CAMERA_INDEX: {camera_index}')

    if args.dump:
        print("-" * 60)
        print('NOTE: Running in DUMP mode.')
        print('      While in dump mode, you need to use a separate rsid-client to initiate authentication for the'
              '      RAW image to appear on this viewer.')
        print("-" * 60)

    config = None
    with rsid_py.FaceAuthenticator(str(port)) as f:
        try:
            config = copy.copy(f.query_device_config())
            if args.dump:
                config.dump_mode = rsid_py.DumpMode.FullFrame
                f.set_device_config(config)
            elif args.crop:
                config.dump_mode = rsid_py.DumpMode.CroppedFace
                f.set_device_config(config)
            else:
                config.dump_mode = rsid_py.DumpMode.Disable
                f.set_device_config(config)
        except Exception as e:
            print("Exception")
            print("-" * 60)
            traceback.print_exc(file=sys.stdout)
            print("-" * 60)
            os._exit(1)
        finally:
            f.disconnect()

    def signal_handler(sig, frame):
        gui.exit_app()

    signal.signal(signal.SIGINT, signal_handler)

    controller = Controller(port=port, camera_index=camera_index, dump_mode=config.dump_mode)
    controller.daemon = True
    controller.start()
    gui = GUI(controller)
    gui.mainloop()


if __name__ == '__main__':
    if sys.platform.startswith('win'):
        app_id = 'intel.realsenseid.viewer.1.0'
        try:
            ctypes.windll.shcore.SetProcessDpiAwareness(1)
            ctypes.windll.shell32.SetCurrentProcessExplicitAppUserModelID(app_id)
        except:
            ctypes.windll.user32.SetProcessDPIAware()

    main()
