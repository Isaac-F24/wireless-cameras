# Each camera POSTs raw JPEG to /upload/<cam_id>
# View per‑camera MJPEG at /stream/<cam_id>
# GET /cameras returns a JSON list of cameras that have posted at least one frame.
import time
import logging
from threading import Lock, Thread
from flask import Flask, request, Response, abort, jsonify, render_template

app = Flask(__name__)

_frames: dict[str, bytes] = {}
_last_update_time: dict[str, float] = {} # time of last update for each camera
_live_cams: set[str] = set()  # set of cameras that are currently live
_lock = Lock()


class PostLogFilter(logging.Filter):
    def filter(self, record):
        return ("POST /upload" not in record.getMessage())
logging.getLogger('werkzeug').addFilter(PostLogFilter())
    

@app.route('/')
def index():
    return render_template('index.html', live_cams=_live_cams)

# Camera upload endpoint
@app.route('/upload/<cam_id>', methods=['POST'])
def upload(cam_id: str):
    # Minimal validation
    if not request.data:
        abort(400, description='Empty frame')
    if len(request.data) < 100:           # probably not a JPEG
        abort(400, description='Frame too small')

    with _lock:
        _frames[cam_id] = request.data
        _last_update_time[cam_id] = time.time()
        if cam_id not in _live_cams:
            print(f"Camera {cam_id} started streaming")
        _live_cams.add(cam_id)
    return ('', 204)                      # ESP32 only checks for 200/204



def _mjpeg_generator(cam_id: str):
    boundary = b'--frame'
    sleep_s  = 0.03                       # yield ≤~30 fps to viewer

    while True:
        with _lock:
            frame = _frames.get(cam_id)
        if frame is not None:
            yield (
                boundary +
                b'\r\nContent-Type: image/jpeg\r\nContent-Length: ' +
                str(len(frame)).encode() +
                b'\r\n\r\n' + frame + b'\r\n'
            )
        time.sleep(sleep_s)


@app.route('/stream/<cam_id>')
def stream(cam_id: str):
    response =  Response(
        _mjpeg_generator(cam_id),
        mimetype='multipart/x-mixed-replace; boundary=frame'
    )

    return response

@app.route('/cameras')
def cameras():
    with _lock:
        cams = list(_frames.keys())
    return jsonify(cameras=cams, count=len(cams))


def check_camera_status_loop():

    TIMEOUT = 5  # seconds

    while True:
        with _lock:
            # Remove cameras that haven't posted in 5 seconds
            for cam_id, last_update_time in list(_last_update_time.items()):
                if time.time() - last_update_time > TIMEOUT:
                    print(f"Camera {cam_id} timed out")

                    # replace the frame with a placeholder image
                    with open('static/images/no-connection.jpg', 'rb') as f:
                        data = f.read()

                    _frames[cam_id] = data
                    del _last_update_time[cam_id]
                    _live_cams.discard(cam_id)

        time.sleep(1)  # Check every second


if __name__ == '__main__':
    # Example:  python backend_server_multi.py --port 5000
    import argparse, os
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', default='0.0.0.0')
    parser.add_argument('--port', type=int, default=int(os.getenv('PORT', 33060)))
    args = parser.parse_args()

    camera_check_thread = Thread(target=check_camera_status_loop, daemon=True)
    camera_check_thread.start()

    app.run(host=args.host, port=args.port, threaded=True)
