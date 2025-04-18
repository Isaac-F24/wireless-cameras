import express from 'express'
import fs from "fs"
import path from "path"
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const app = express()
const port = 3000
const boundary = "aa55aa"

async function sleep(millis) {
    return new Promise(resolve => setTimeout(resolve, millis));
}

app.get('/stream', async (req, res) => {
  res.set("Content-Type", `multipart/x-mixed-replace;boundary=${boundary}`)
  for (let index = 1; index < 4236; index++) {
    const fileName = `test-jpegs/${String(index).padStart(3, "0")}.jpg`
    const data = fs.readFileSync(fileName)
    res.write(`\r\n--${boundary}\r\n`)
    res.write(`Content-Type: image/jpeg\r\nContent-Length: ${data.length}\r\n\r\n`)
    res.write(data)
    await sleep(30)
  }
})

app.get("/", (req, res) => {
  res.sendFile(path.join(__dirname, "index.html"))
})

app.listen(port, () => {
  console.log(`Example app listening on port ${port}`)
})
