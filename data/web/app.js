function switchTab(tabId) {
  document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
  document.querySelectorAll('.tab-content').forEach(c => c.style.display = 'none');

  // Support both onclick event and programmatic calls via data-tab attribute
  const tab = document.querySelector(`.tab[data-tab="${tabId}"]`);
  if (tab) tab.classList.add('active');
  document.getElementById('tab-' + tabId).style.display = 'block';
}

async function loadTelemetry() {
  try {
    const res = await fetch('/api/status');
    const data = await res.json();

    document.getElementById('poolTemp').textContent = isFinite(data.pool_temp) ? data.pool_temp.toFixed(1) + ' °C' : '-- °C';
    document.getElementById('solarTemp').textContent = isFinite(data.solar_temp) ? data.solar_temp.toFixed(1) + ' °C' : '-- °C';

    const pb = document.getElementById('poolPump');
    pb.textContent = data.pool_pump ? 'RUNNING' : 'OFF';
    pb.style.color = data.pool_pump ? 'var(--accent-blue)' : 'var(--text-muted)';

    const sb = document.getElementById('solarPump');
    sb.textContent = data.solar_pump ? 'RUNNING' : 'OFF';
    sb.style.color = data.solar_pump ? 'var(--accent-solar)' : 'var(--text-muted)';

    document.getElementById('heapVal').textContent = (data.free_heap / 1024).toFixed(1) + ' KB';
    document.getElementById('rssiVal').textContent = data.rssi + ' dBm';
    const uptime = data.uptime;
    const days = Math.floor(uptime / 86400);
    const hours = Math.floor((uptime % 86400) / 3600);
    const mins = Math.floor((uptime % 3600) / 60);
    document.getElementById('uptimeVal').textContent = days + 'd ' + hours + 'h ' + mins + 'm';
    // Display firmware version
    document.getElementById('fwVersionDisplay').textContent = data.fw_version || '--';
    // Update active mode card on dashboard
    highlightMode(data.op_mode);
  } catch (e) {
    console.error("Telemetry failed to refresh");
  }
}

async function scanNetworks() {
  const select = document.getElementById('wifiSelect');
  select.innerHTML = '<option>Scanning available networks...</option>';
  const res = await fetch('/api/scan');
  const data = await res.json();

  select.innerHTML = '<option value="">-- Choose Network --</option>';
  data.forEach(n => {
    const opt = document.createElement('option');
    opt.value = n.ssid;
    opt.textContent = n.ssid + ' (' + n.rssi + ' dBm) ' + (n.secure ? '🔒' : '🔓');
    select.appendChild(opt);
  });
}

async function saveWiFi() {
  const ssid = document.getElementById('wifiSsid').value.trim();
  if (!ssid) { alert('Please enter or select a WiFi SSID.'); return; }
  const pass = document.getElementById('wifiPass').value;

  const res = await fetch('/api/config', {
    method: 'POST',
    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
    body: 'type=wifi&ssid=' + encodeURIComponent(ssid) + '&password=' + encodeURIComponent(pass)
  });
  if(res.status===200) {
    alert("WiFi config saved! Device is rebooting to connect...");
    setTimeout(() => window.location.reload(), 3000);
  }
}

async function saveMqtt() {
  const host = document.getElementById('mqttHost').value.trim();
  if (!host) { alert('Please enter an MQTT hostname or IP address.'); return; }
  const portVal = parseInt(document.getElementById('mqttPort').value, 10);
  if (isNaN(portVal) || portVal < 1 || portVal > 65535) { alert('MQTT Port must be a number between 1 and 65535.'); return; }
  const user = document.getElementById('mqttUser').value;
  const pass = document.getElementById('mqttPass').value;
  const tls = document.getElementById('mqttTls').checked;

  const res = await fetch('/api/config', {
    method: 'POST',
    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
    body: 'type=mqtt&host=' + encodeURIComponent(host) + '&port=' + portVal + '&username=' + encodeURIComponent(user) + '&password=' + encodeURIComponent(pass) + '&tls=' + tls
  });
  if(res.status===200) alert("MQTT config saved!");
}

async function setMode(mode) {
  const fb = document.getElementById('modeFeedback');
  try {
    const res = await fetch('/api/mode', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: 'mode=' + encodeURIComponent(mode)
    });
    const data = await res.json();
    if (data.status === 'ok') {
      highlightMode(mode);
      fb.textContent = '✓ Mode switched to ' + mode;
      fb.className = 'mode-feedback mode-feedback-ok';
      fb.style.display = 'block';
      setTimeout(() => { fb.style.display = 'none'; }, 3000);
    } else {
      fb.textContent = '✖ ' + (data.message || 'Switch failed');
      fb.className = 'mode-feedback mode-feedback-err';
      fb.style.display = 'block';
    }
  } catch (e) {
    fb.textContent = '✖ Network error';
    fb.className = 'mode-feedback mode-feedback-err';
    fb.style.display = 'block';
  }
  // Also update the dropdown in the settings tab
  document.getElementById('opMode').value = mode;
}

function highlightMode(mode) {
  document.querySelectorAll('.mode-card').forEach(c => c.classList.remove('active'));
  const card = document.querySelector('.mode-card[data-mode="' + mode + '"]');
  if (card) card.classList.add('active');
}

// Helper: build timer params from two time inputs
function timerParams() {
  const [sh, sm] = document.getElementById('timerStart').value.split(':');
  const [eh, em] = document.getElementById('timerEnd').value.split(':');
  return '&timer_start_h=' + sh + '&timer_start_m=' + sm + '&timer_end_h=' + eh + '&timer_end_m=' + em;
}

async function setMinSolarTemp() {
  const currentMin = document.getElementById('tempMinSolar').value;
  const newVal = prompt('Enter minimum solar temperature (°C):', currentMin);
  if (newVal === null || newVal === '') return;
  const val = parseFloat(newVal);
  if (isNaN(val) || val < 0 || val > 90) {
    alert('Invalid value. Please enter a temperature between 0 and 90 °C.');
    return;
  }
  const res = await fetch('/api/config', {
    method: 'POST',
    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
    body: 'type=settings&min_solar=' + val + '&mode=' + document.getElementById('opMode').value + '&interval=' + document.getElementById('loopInterval').value + '&max_pool=' + document.getElementById('tempMaxPool').value + '&hysteresis=' + document.getElementById('tempHysteresis').value + '&timezone=' + document.getElementById('timezone').value + '&green=' + document.getElementById('timeLossGreen').value + '&red=' + document.getElementById('timeLossRed').value + timerParams()
  });
  if (res.status === 200) {
    document.getElementById('tempMinSolar').value = val;
    document.getElementById('solarThreshold').textContent = 'min ' + val.toFixed(1) + '°C';
    alert('Minimum solar temperature set to ' + val.toFixed(1) + ' °C');
  } else {
    alert('Failed to save minimum solar temperature.');
  }
}

async function setMaxPoolTemp() {
  const currentMax = document.getElementById('tempMaxPool').value;
  const newVal = prompt('Enter maximum pool temperature (°C):', currentMax);
  if (newVal === null || newVal === '') return;
  const val = parseFloat(newVal);
  if (isNaN(val) || val < 0 || val > 60) {
    alert('Invalid value. Please enter a temperature between 0 and 60 °C.');
    return;
  }
  const res = await fetch('/api/config', {
    method: 'POST',
    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
    body: 'type=settings&max_pool=' + val + '&mode=' + document.getElementById('opMode').value + '&interval=' + document.getElementById('loopInterval').value + '&min_solar=' + document.getElementById('tempMinSolar').value + '&hysteresis=' + document.getElementById('tempHysteresis').value + '&timezone=' + document.getElementById('timezone').value + '&green=' + document.getElementById('timeLossGreen').value + '&red=' + document.getElementById('timeLossRed').value + timerParams()
  });
  if (res.status === 200) {
    document.getElementById('tempMaxPool').value = val;
    document.getElementById('poolThreshold').textContent = 'max ' + val.toFixed(1) + '°C';
    alert('Maximum pool temperature set to ' + val.toFixed(1) + ' °C');
  } else {
    alert('Failed to save maximum pool temperature.');
  }
}

function validateSettings() {
  const fields = [
    { id: 'loopInterval',    name: 'Loop Interval',       min: 1,   max: 300,  type: 'int' },
    { id: 'tempMaxPool',     name: 'Max Pool Temp',       min: 0,   max: 40,   type: 'float' },
    { id: 'tempMinSolar',    name: 'Min Solar Temp',      min: 0,   max: 90,   type: 'float' },
    { id: 'tempHysteresis',  name: 'Hysteresis',          min: 0,   max: 10,   type: 'float' },
    { id: 'timeLossGreen',   name: 'Time Sync Green',     min: 1,   max: 6,    type: 'int' },
    { id: 'timeLossRed',     name: 'Time Sync Red',       min: 1,   max: 72,   type: 'int' },
  ];
  for (const f of fields) {
    const el = document.getElementById(f.id);
    const val = f.type === 'float' ? parseFloat(el.value) : parseInt(el.value, 10);
    if (isNaN(val) || val < f.min || val > f.max) {
      alert(f.name + ' must be between ' + f.min + ' and ' + f.max + '.');
      el.focus();
      el.select();
      return false;
    }
  }
  // Validate time inputs
  for (const id of ['timerStart', 'timerEnd']) {
    const el = document.getElementById(id);
    if (!el.value.match(/^\d{2}:\d{2}$/)) {
      alert('Invalid time format.');
      el.focus();
      return false;
    }
  }
  return true;
}

async function saveControllerSettings() {
  if (!validateSettings()) return;

  const mode = document.getElementById('opMode').value;
  const interval = document.getElementById('loopInterval').value;
  const maxPool = document.getElementById('tempMaxPool').value;
  const minSolar = document.getElementById('tempMinSolar').value;
  const hysteresis = document.getElementById('tempHysteresis').value;
  const tz = document.getElementById('timezone').value;
  const green = document.getElementById('timeLossGreen').value;
  const red = document.getElementById('timeLossRed').value;

  const res = await fetch('/api/config', {
    method: 'POST',
    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
    body: 'type=settings&mode=' + mode + '&interval=' + interval + '&max_pool=' + maxPool + '&min_solar=' + minSolar + '&hysteresis=' + hysteresis + '&timezone=' + tz + '&green=' + green + '&red=' + red + timerParams()
  });
  if(res.status===200) {
    // Refresh displayed thresholds with saved values
    document.getElementById('poolThreshold').textContent = 'max ' + parseFloat(maxPool).toFixed(1) + '°C';
    document.getElementById('solarThreshold').textContent = 'min ' + parseFloat(minSolar).toFixed(1) + '°C';
    alert("Controller setpoints saved!");
    highlightMode(mode);
  }
}

async function savePassword() {
  const pass = document.getElementById('adminPass').value;
  if (!pass || pass.length < 4) { alert('Password must be at least 4 characters.'); return; }
  const res = await fetch('/api/config', {
    method: 'POST',
    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
    body: 'type=password&password=' + encodeURIComponent(pass)
  });
  if(res.status===200) {
    alert("Admin Password updated!");
    document.getElementById('adminPass').value = '';
  }
}

async function restartDevice() {
  if(confirm("Confirm restart?")) {
    fetch('/api/restart');
    alert("ESP is rebooting. Reloading soon.");
    setTimeout(() => window.location.reload(), 4000);
  }
}

async function factoryReset() {
  if(confirm("DANGER: WIPE config file and factory reset device?")) {
    fetch('/api/factory_reset');
    alert("Config deleted. Rebooting into AP setup mode.");
    setTimeout(() => window.location.reload(), 4000);
  }
}

async function loadConfig() {
  const res = await fetch('/api/config');
  const data = await res.json();

  document.getElementById('wifiSsid').value = data.wifi.ssid;
  document.getElementById('mqttHost').value = data.mqtt.host;
  document.getElementById('mqttPort').value = data.mqtt.port;
  document.getElementById('mqttUser').value = data.mqtt.username;
  document.getElementById('mqttTls').checked = data.mqtt.use_tls;

  document.getElementById('opMode').value = data.settings.op_mode;
  document.getElementById('loopInterval').value = data.settings.loop_interval;
  document.getElementById('tempMaxPool').value = data.settings.temp_max_pool;
  document.getElementById('tempMinSolar').value = data.settings.temp_min_solar;
  document.getElementById('tempHysteresis').value = data.settings.temp_hysteresis;
  document.getElementById('timezone').value = data.settings.timezone;
  document.getElementById('timeLossGreen').value = data.settings.time_loss_green_hours;
  document.getElementById('timeLossRed').value = data.settings.time_loss_red_hours;
  const pad2 = (n) => n.toString().padStart(2, '0');
  document.getElementById('timerStart').value = pad2(data.settings.timer_start_hour) + ':' + pad2(data.settings.timer_start_min);
  document.getElementById('timerEnd').value = pad2(data.settings.timer_end_hour) + ':' + pad2(data.settings.timer_end_min);
  document.getElementById('poolThreshold').textContent = 'max ' + data.settings.temp_max_pool.toFixed(1) + '°C';
  document.getElementById('solarThreshold').textContent = 'min ' + data.settings.temp_min_solar.toFixed(1) + '°C';
  highlightMode(data.settings.op_mode);
}

// ── Firmware Update ──

async function checkFirmwareUpdate() {
  const btn = document.getElementById('btnCheckUpdate');
  const statusEl = document.getElementById('fwUpdateStatus');
  btn.textContent = 'Checking...';
  btn.disabled = true;
  statusEl.style.display = 'none';

  try {
    const res = await fetch('/api/update/check');
    const data = await res.json();

    document.getElementById('fwCurrentVersion').textContent = data.current_version || '--';
    document.getElementById('fwLatestVersion').textContent = data.latest_version || '--';

    if (data.status === 'update_available') {
      statusEl.style.display = 'block';
      statusEl.style.background = 'rgba(0, 229, 255, 0.12)';
      statusEl.style.color = '#00e5ff';
      statusEl.textContent = '✨ Update available: v' + data.latest_version;
      document.getElementById('btnInstallUpdate').style.display = 'block';
    } else if (data.status === 'up_to_date') {
      statusEl.style.display = 'block';
      statusEl.style.background = 'rgba(34, 197, 94, 0.12)';
      statusEl.style.color = '#22c55e';
      statusEl.textContent = '✓ Firmware is up to date (v' + (data.current_version || '--') + ')';
    }
  } catch (e) {
    statusEl.style.display = 'block' ;
    statusEl.style.background = 'rgba(239, 68, 68, 0.12)';
    statusEl.style.color = '#ef4444';
    statusEl.textContent = '✖ Check failed — is WiFi connected?';
  }

  btn.textContent = 'Check for Updates';
  btn.disabled = false;
}

async function installFirmwareUpdate() {
  if (!confirm('Start firmware update? The device will reboot and be unavailable for a few minutes.')) {
    return;
  }

  document.getElementById('btnInstallUpdate').style.display = 'none';
  document.getElementById('fwUpdateStatus').style.display = 'none';
  document.getElementById('fwProgressContainer').style.display = 'block';
  document.getElementById('btnCheckUpdate').disabled = true;

  try {
    const res = await fetch('/api/update/install', { method: 'POST' });
    // Response comes before reboot
    if (!res.ok) {
      const data = await res.json();
      alert('Update failed: ' + (data.message || 'Unknown error'));
      resetUpdateUI();
      return;
    }
  } catch (e) {
    // If the ESP reboots during the request, fetch may fail — that's expected
    console.log('Update triggered, device may be rebooting...');
  }

  // Poll for status while update is running
  pollUpdateProgress();
}

async function pollUpdateProgress() {
  const interval = setInterval(async () => {
    try {
      const res = await fetch('/api/update/status');
      const data = await res.json();

      const progress = data.progress || 0;
      document.getElementById('fwProgressBar').style.width = progress + '%';
      document.getElementById('fwProgressPercent').textContent = progress + '%';

      if (data.status_message) {
        document.getElementById('fwProgressLabel').textContent = data.status_message;
      }

      if (!data.update_in_progress) {
        clearInterval(interval);
        if (data.update_available) {
          document.getElementById('fwProgressLabel').textContent = 'Update failed';
          document.getElementById('fwProgressBar').style.background = 'var(--danger)';
        } else {
          document.getElementById('fwProgressLabel').textContent = '✓ Update complete! Rebooting...';
          document.getElementById('fwProgressBar').style.width = '100%';
          document.getElementById('fwProgressBar').style.background = '#22c55e';
        }
        setTimeout(resetUpdateUI, 5000);
      }
    } catch (e) {
      // ESP likely rebooting — connection lost is expected
      clearInterval(interval);
      document.getElementById('fwProgressLabel').textContent = 'Rebooting...';
      // After reboot, reload the page
      setTimeout(() => window.location.reload(), 15000);
    }
  }, 1000);
}

function resetUpdateUI() {
  document.getElementById('fwProgressContainer').style.display = 'none';
  document.getElementById('btnCheckUpdate').disabled = false;
  document.getElementById('btnInstallUpdate').style.display = 'none';
  // Re-check version after reset
  setTimeout(() => {
    fetch('/api/update/status')
      .then(r => r.json())
      .then(data => {
        document.getElementById('fwCurrentVersion').textContent = data.current_version || '--';
        document.getElementById('fwLatestVersion').textContent = data.latest_version || '--';
      })
      .catch(() => {});
  }, 2000);
}

setInterval(loadTelemetry, 2000);
window.onload = () => {
  loadTelemetry();
  loadConfig();
  // Show current version from telemetry endpoint
  setTimeout(() => {
    fetch('/api/status')
      .then(r => r.json())
      .then(data => {
        if (data.fw_version) {
          document.getElementById('fwCurrentVersion').textContent = data.fw_version;
        }
        // In AP mode, show WiFi configuration tab directly
        if (data.ap_mode) {
          switchTab('wifi');
        }
      })
      .catch(() => {});
  }, 500);
};
