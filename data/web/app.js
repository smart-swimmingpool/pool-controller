// ── Tab Switching ──

function switchTab(tabName) {
  // Hide all tab contents
  document.querySelectorAll('.tab-content').forEach(c => c.style.display = 'none');

  // Show the requested tab
  const tab = document.getElementById('tab-' + tabName);
  if (tab) {
    tab.style.display = 'block';
  }

  // Close settings menu if open
  const menu = document.getElementById('settingsMenu');
  if (menu) menu.style.display = 'none';
}

function toggleSettingsMenu() {
  const menu = document.getElementById('settingsMenu');
  if (menu) {
    menu.style.display = menu.style.display === 'none' || menu.style.display === '' ? 'block' : 'none';
  }
}

// Close settings menu when clicking outside
document.addEventListener('click', function(event) {
  const menu = document.getElementById('settingsMenu');
  const cog = document.querySelector('.cogwheel');
  if (menu && menu.style.display === 'block') {
    if (!menu.contains(event.target) && !cog.contains(event.target)) {
      menu.style.display = 'none';
    }
  }
});

// ── Telemetry ──

console.log('[pool] app.js loaded, version=2026-06-05');

async function loadTelemetry() {
  try {
    const res = await fetch('/api/status');
    const data = await res.json();
    console.log('[pool] loadTelemetry got data:', JSON.stringify(data));

    // Temperaturen
    if (data.pool_temp != null) {
      document.getElementById('poolTemp').textContent = data.pool_temp.toFixed(1) + ' °C';
    }
    if (data.solar_temp != null) {
      document.getElementById('solarTemp').textContent = data.solar_temp.toFixed(1) + ' °C';
    }

    // Pumpen
    if (data.pool_pump != null) {
      const el = document.getElementById('poolPump');
      const isOn = data.pool_pump;
      el.innerHTML = isOn
        ? '<span style="color: #22c55e;">●</span> ON'
        : '<span style="color: var(--text-muted);">○</span> OFF';
      el.style.color = isOn ? '#22c55e' : 'var(--text-muted)';
    }
    if (data.solar_pump != null) {
      const el = document.getElementById('solarPump');
      const isOn = data.solar_pump;
      el.innerHTML = isOn
        ? '<span style="color: #22c55e;">●</span> ON'
        : '<span style="color: var(--text-muted);">○</span> OFF';
      el.style.color = isOn ? '#22c55e' : 'var(--text-muted)';
    }

    // Modus hervorheben
    if (data.op_mode) {
      highlightMode(data.op_mode);
    }

    // Firmware-Version
    if (data.fw_version) {
      document.getElementById('fwCurrentVersion').textContent = data.fw_version;
      document.getElementById('fwVersionDisplay').textContent = data.fw_version;
    }

    // Lokale Uhrzeit
    if (data.local_time) {
      const parts = data.local_time.split(' ');
      document.getElementById('localTimeDisplay').textContent = parts[1] || data.local_time;
      document.getElementById('localTimeDate').textContent = parts[0] || '';
      document.getElementById('localTimeZone').textContent = data.timezone_name || '';
      // Farbpunkt basierend auf Zeit-Sync-Status
      const dot = document.getElementById('timeDegradationDot');
      if (data.time_degradation === 0) {
        dot.style.background = '#22c55e';
      } else if (data.time_degradation === 1) {
        dot.style.background = '#eab308';
      } else {
        dot.style.background = '#ef4444';
      }
    }

    // Telemetrie (klein, unten)
    if (data.free_heap != null) {
      document.getElementById('heapVal').textContent = (data.free_heap / 1024).toFixed(0) + ' KB';
    }
    if (data.rssi != null) {
      document.getElementById('rssiVal').textContent = data.rssi + ' dBm';
    }
    if (data.uptime != null) {
      const h = Math.floor(data.uptime / 3600);
      const m = Math.floor((data.uptime % 3600) / 60);
      document.getElementById('uptimeVal').textContent = h + 'h ' + m + 'm';
    }

    // AP-Mode: WiFi-Tab anzeigen
    if (data.ap_mode) {
      switchTab('wifi');
    }
  } catch (e) {
    console.error('[pool] loadTelemetry error:', e);
  }
}

// ── WiFi Scan ──

async function scanNetworks() {
  const select = document.getElementById('wifiSelect');
  select.innerHTML = '<option>Scanning available networks...</option>';
  try {
    const res = await fetch('/api/scan');
    const data = await res.json();
    select.innerHTML = '<option value="">-- Choose Network --</option>';
    data.forEach(n => {
      const opt = document.createElement('option');
      opt.value = n.ssid;
      opt.textContent = n.ssid + ' (' + n.rssi + ' dBm) ' + (n.secure ? '🔒' : '🔓');
      select.appendChild(opt);
    });
  } catch (e) {
    select.innerHTML = '<option value="">Scan failed</option>';
  }
}

// ── WiFi Save ──

async function saveWiFi() {
  const ssid = document.getElementById('wifiSsid').value.trim();
  if (!ssid) { alert('Please enter or select a WiFi SSID.'); return; }
  const pass = document.getElementById('wifiPass').value;
  const res = await fetch('/api/config', {
    method: 'POST',
    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
    body: 'type=wifi&ssid=' + encodeURIComponent(ssid) + '&password=' + encodeURIComponent(pass)
  });
  if (res.status === 200) {
    alert('WiFi config saved! Device is rebooting to connect...');
    setTimeout(() => window.location.reload(), 3000);
  }
}

// ── MQTT Save ──

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
  if (res.status === 200) alert('MQTT config saved!');
}

// ── Pump Toggle ──

async function togglePump(pump) {
  try {
    const res = await fetch('/api/pump', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: 'pump=' + encodeURIComponent(pump)
    });
    const data = await res.json();
    if (data.status !== 'ok') {
      alert('✖ ' + (data.message || 'Cannot toggle pump'));
    }
  } catch (e) {
    // State will be picked up on next telemetry poll
  }
}

// ── Mode Switching ──

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
      fb.style.background = 'rgba(34, 197, 94, 0.15)';
      fb.style.color = '#22c55e';
      setTimeout(() => { fb.style.display = 'none'; }, 3000);
    } else {
      fb.textContent = '✖ ' + (data.message || 'Switch failed');
      fb.className = 'mode-feedback mode-feedback-err';
      fb.style.display = 'block';
      fb.style.background = 'rgba(239, 68, 68, 0.15)';
      fb.style.color = '#ef4444';
    }
  } catch (e) {
    fb.textContent = '✖ Network error';
    fb.className = 'mode-feedback mode-feedback-err';
    fb.style.display = 'block';
    fb.style.background = 'rgba(239, 68, 68, 0.15)';
    fb.style.color = '#ef4444';
  }
  document.getElementById('opMode').value = mode;
}

function highlightMode(mode) {
  document.querySelectorAll('.mode-card').forEach(c => {
    c.classList.remove('active');
    c.style.background = 'var(--panel-bg)';
    c.style.borderColor = 'var(--panel-border)';
  });
  const card = document.querySelector('.mode-card[data-mode="' + mode + '"]');
  if (card) {
    card.classList.add('active');
    card.style.background = 'rgba(0, 229, 255, 0.12)';
    card.style.borderColor = 'var(--accent-solar)';
  }
}

// ── Helper: timer params ──

function timerParams() {
  const [sh, sm] = document.getElementById('timerStart').value.split(':');
  const [eh, em] = document.getElementById('timerEnd').value.split(':');
  return '&timer_start_h=' + sh + '&timer_start_m=' + sm + '&timer_end_h=' + eh + '&timer_end_m=' + em;
}

// ── Set Temperatures ──

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

// ── Validate Settings ──

function validateSettings() {
  const fields = [
    { id: 'loopInterval',    name: 'Loop Interval',       min: 1,   max: 300,  type: 'int' },
    { id: 'tempMaxPool',     name: 'Max Pool Temp',       min: 0,   max: 40,   type: 'float' },
    { id: 'tempMinSolar',    name: 'Min Solar Temp',      min: 0,   max: 90,   type: 'float' },
    { id: 'tempHysteresis',  name: 'Hysteresis',          min: 0,   max: 10,   type: 'float' },
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

// ── Save Controller Settings (Pool Tab) ──

async function saveControllerSettings() {
  if (!validateSettings()) return;

  const mode = document.getElementById('opMode').value;
  const interval = document.getElementById('loopInterval').value;
  const maxPool = document.getElementById('tempMaxPool').value;
  const minSolar = document.getElementById('tempMinSolar').value;
  const hysteresis = document.getElementById('tempHysteresis').value;
  const tz = document.getElementById('timezone').value;

  // Validate time fields included alongside pool fields
  const green = parseInt(document.getElementById('timeLossGreen').value, 10);
  const red = parseInt(document.getElementById('timeLossRed').value, 10);
  if (isNaN(green) || green < 1 || green > 6) {
    alert('Time Sync Green must be between 1 and 6 hours.');
    document.getElementById('timeLossGreen').focus();
    return;
  }
  if (isNaN(red) || red < 1 || red > 72) {
    alert('Time Sync Red must be between 1 and 72 hours.');
    document.getElementById('timeLossRed').focus();
    return;
  }
  const ntpServer = encodeURIComponent(document.getElementById('ntpServer').value);

  const res = await fetch('/api/config', {
    method: 'POST',
    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
    body: 'type=settings&mode=' + mode + '&interval=' + interval + '&max_pool=' + maxPool + '&min_solar=' + minSolar + '&hysteresis=' + hysteresis + '&timezone=' + tz + '&green=' + green + '&red=' + red + timerParams() + '&ntp_server=' + ntpServer
  });
  if (res.status === 200) {
    document.getElementById('poolThreshold').textContent = 'max ' + parseFloat(maxPool).toFixed(1) + '°C';
    document.getElementById('solarThreshold').textContent = 'min ' + parseFloat(minSolar).toFixed(1) + '°C';
    alert('✓ Pool settings saved!');
    highlightMode(mode);
  }
}

// ── Save Time Settings (Time Tab) ──

async function saveTimeSettings() {
  // Validate pool fields (read alongside the request)
  if (!validateSettings()) return;

  const tz = document.getElementById('timezone').value;
  const ntpServer = encodeURIComponent(document.getElementById('ntpServer').value);
  const green = parseInt(document.getElementById('timeLossGreen').value, 10);
  const red = parseInt(document.getElementById('timeLossRed').value, 10);

  if (isNaN(green) || green < 1 || green > 6) {
    alert('Green Threshold must be between 1 and 6 hours.');
    document.getElementById('timeLossGreen').focus();
    return;
  }
  if (isNaN(red) || red < 1 || red > 72) {
    alert('Red Threshold must be between 1 and 72 hours.');
    document.getElementById('timeLossRed').focus();
    return;
  }

  // Read all pool fields to preserve them on save
  const mode = document.getElementById('opMode').value;
  const interval = document.getElementById('loopInterval').value;
  const maxPool = document.getElementById('tempMaxPool').value;
  const minSolar = document.getElementById('tempMinSolar').value;
  const hysteresis = document.getElementById('tempHysteresis').value;

  const res = await fetch('/api/config', {
    method: 'POST',
    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
    body: 'type=settings&mode=' + mode + '&interval=' + interval + '&max_pool=' + maxPool + '&min_solar=' + minSolar + '&hysteresis=' + hysteresis + '&timezone=' + tz + '&green=' + green + '&red=' + red + timerParams() + '&ntp_server=' + ntpServer
  });
  if (res.status === 200) {
    alert('✓ Time settings saved!');
  }
}

// ── Save Password ──

async function savePassword() {
  const pass = document.getElementById('adminPass').value;
  if (!pass || pass.length < 4) { alert('Password must be at least 4 characters.'); return; }
  const res = await fetch('/api/config', {
    method: 'POST',
    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
    body: 'type=password&password=' + encodeURIComponent(pass)
  });
  if (res.status === 200) {
    alert('Admin Password updated!');
    document.getElementById('adminPass').value = '';
  }
}

// ── Device Actions ──

async function restartDevice() {
  if (confirm('Confirm restart?')) {
    fetch('/api/restart');
    alert('ESP is rebooting. Reloading soon.');
    setTimeout(() => window.location.reload(), 4000);
  }
}

async function factoryReset() {
  if (confirm('DANGER: WIPE config file and factory reset device?')) {
    fetch('/api/factory_reset');
    alert('Config deleted. Rebooting into AP setup mode.');
    setTimeout(() => window.location.reload(), 4000);
  }
}

// ── Load Config ──

async function loadConfig() {
  try {
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
    document.getElementById('ntpServer').value = data.ntp.server;
    document.getElementById('timeLossGreen').value = data.settings.time_loss_green_hours;
    document.getElementById('timeLossRed').value = data.settings.time_loss_red_hours;
    const pad2 = (n) => n.toString().padStart(2, '0');
    document.getElementById('timerStart').value = pad2(data.settings.timer_start_hour) + ':' + pad2(data.settings.timer_start_min);
    document.getElementById('timerEnd').value = pad2(data.settings.timer_end_hour) + ':' + pad2(data.settings.timer_end_min);
    document.getElementById('poolThreshold').textContent = 'max ' + data.settings.temp_max_pool.toFixed(1) + '°C';
    document.getElementById('solarThreshold').textContent = 'min ' + data.settings.temp_min_solar.toFixed(1) + '°C';
    highlightMode(data.settings.op_mode);
  } catch (e) {
    // Silent
  }
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
    statusEl.style.display = 'block';
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
    if (!res.ok) {
      const data = await res.json();
      alert('Update failed: ' + (data.message || 'Unknown error'));
      resetUpdateUI();
      return;
    }
  } catch (e) {
    console.log('Update triggered, device may be rebooting...');
  }

  pollUpdateProgress();
}

function resetUpdateUI() {
  document.getElementById('fwProgressContainer').style.display = 'none';
  document.getElementById('btnCheckUpdate').disabled = false;
  document.getElementById('btnInstallUpdate').style.display = 'none';
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
      clearInterval(interval);
      document.getElementById('fwProgressLabel').textContent = 'Rebooting...';
      setTimeout(() => window.location.reload(), 15000);
    }
  }, 1000);
}

// ── Init ──

setInterval(loadTelemetry, 2000);

window.onload = function() {
  loadTelemetry();
  loadConfig();
};
