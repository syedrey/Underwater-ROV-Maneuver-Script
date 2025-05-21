const express = require('express');
const { NodeSSH } = require('node-ssh');

const app = express();
const PORT = 3000;

const ssh = new NodeSSH();

// SSH Configuration
const sshConfig = {
  host: '192.168.1.11', // Replace with your Raspberry Pi's IP
  username: 'sreyas',        // Replace with your Raspberry Pi's username
  password: '1234', // Replace with your Raspberry Pi's password
};

// Connect to Raspberry Pi via SSH
ssh.connect(sshConfig)
  .then(() => {
    console.log('SSH connection established');
  })
  .catch((err) => {
    console.error('SSH connection failed:', err);
  });

// Endpoint to restart Raspberry Pi
app.post('/restart', async (req, res) => {
  try {
    await ssh.execCommand('sudo reboot');
    res.status(200).send('Raspberry Pi is restarting...');
  } catch (err) {
    res.status(500).send('Failed to restart Raspberry Pi');
  }
});

// Endpoint to shutdown Raspberry Pi
app.post('/shutdown', async (req, res) => {
  try {
    await ssh.execCommand('sudo shutdown now');
    res.status(200).send('Raspberry Pi is shutting down...');
  } catch (err) {
    res.status(500).send('Failed to shutdown Raspberry Pi');
  }
});

app.listen(PORT, () => {
  console.log(`Server running on http://localhost:${PORT}`);
});
