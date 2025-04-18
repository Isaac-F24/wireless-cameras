# Each camera POSTs raw JPEG to /upload/<cam_id>
# View per‑camera MJPEG at /stream/<cam_id>
# GET /cameras returns a JSON list of cameras that have posted at least one frame.
import time
from threading import Lock
from flask import Flask, request, Response, abort, jsonify

app = Flask(__name__)

_frames: dict[str, bytes] = {}
_lock = Lock()

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
    return Response(
        _mjpeg_generator(cam_id),
        mimetype='multipart/x-mixed-replace; boundary=frame'
    )

@app.route('/cameras')
def cameras():
    with _lock:
        cams = list(_frames.keys())
    return jsonify(cameras=cams, count=len(cams))


if __name__ == '__main__':
    # Example:  python backend_server_multi.py --port 5000
    import argparse, os
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', default='0.0.0.0')
    parser.add_argument('--port', type=int, default=int(os.getenv('PORT', 5001)))
    args = parser.parse_args()

    app.run(host=args.host, port=args.port, threaded=True)
