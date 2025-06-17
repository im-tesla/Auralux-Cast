const child_process = require('child_process');
const exec = child_process.exec;

const ipInput = document.getElementById('ip');
const portInput = document.getElementById('port');
const deviceList = document.getElementById('deviceList');
const startBtn = document.getElementById('startBtn');
const stopBtn = document.getElementById('stopBtn');
const listBtn = document.getElementById('listBtn');

let processRef = null;

function listDevices() {
    deviceList.innerHTML = `<option value="-1">(Default)</option>`;

    exec('auracast.exe -list', (err, stdout, stderr) => {
        if (err) {
            alert("Error listing devices");
            return;
        }
        const lines = stdout.split('\n');
        lines.forEach((line, idx) => {
            if (line.trim()) {
                const opt = document.createElement('option');
                opt.value = idx;
                opt.textContent = `[${idx}] ${line.trim()}`;
                deviceList.appendChild(opt);
            }
        });
    });
}

startBtn.onclick = () => {
    const ip = ipInput.value.trim();
    const port = portInput.value.trim();
    const device = deviceList.value;

    const args = [ip, port];
    if (device !== "-1") args.push(device);

    processRef = child_process.spawn('auracast.exe', args, { shell: true });

    processRef.stdout.on('data', data => console.log(data.toString()));
    processRef.stderr.on('data', data => console.error(data.toString()));

    processRef.on('exit', code => {
        console.log(`Auracast exited with code ${code}`);
        stopBtn.disabled = true;
        startBtn.disabled = false;
    });

    startBtn.disabled = true;
    stopBtn.disabled = false;
};

stopBtn.onclick = () => {
    exec('taskkill /f /im auracast.exe', (err, stdout, stderr) => {});
};

window.onload = listDevices;
