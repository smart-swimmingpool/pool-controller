// ── Tab Switching ──

function switchTab(tabName) {
  // Hide all tab contents
  document.querySelectorAll('.tab-content').forEach(c => c.style.display = 'none');

  // Show the requested tab
  const tab = document.getElementById('tab-' + tabName);
  if (tab) {
    tab.style.display = 'block';
  }

  // Update bottom tab bar active state.
  // Tabs under "More" (wifi, mqtt, system, about) keep "more" highlighted.
  const moreTabs = ['logs', 'wifi', 'mqtt', 'system', 'about'];
  const barTab = moreTabs.includes(tabName) ? 'more' : tabName;
  document.querySelectorAll('.tab-bar-item').forEach(item => {
    item.classList.toggle('active', item.dataset.tab === barTab);
  });

  // Close more menu if open
  const moreMenu = document.getElementById('moreMenu');
  if (moreMenu) moreMenu.style.display = 'none';
}

function toggleMoreMenu() {
  const menu = document.getElementById('moreMenu');
  if (menu) {
    menu.style.display = menu.style.display === 'none' || menu.style.display === '' ? 'flex' : 'none';
  }
}

// ── Telemetry ──

console.log('[pool] app.js loaded, version=2026-06-05');

// ── Auth State ──

let isAuthenticated = false;
let hasAutoSwitchedToWifi = false;  // one-shot guard so AP-mode redirect doesn't fight user navigation

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

    // Thresholds (von apiGetStatus, damit sie ohne Auth funktionieren)
    if (data.temp_max_pool != null) {
      document.getElementById('poolThreshold').textContent = 'max ' + data.temp_max_pool.toFixed(1) + '°C';
    }
    if (data.temp_min_solar != null) {
      document.getElementById('solarThreshold').textContent = 'min ' + data.temp_min_solar.toFixed(1) + '°C';
    }

    // Pumpen — Toggle-Switches aktualisieren
    if (data.pool_pump != null) {
      setPumpSwitch('pool', data.pool_pump);
    }
    if (data.solar_pump != null) {
      setPumpSwitch('solar', data.solar_pump);
    }

    // Modus hervorheben
    if (data.op_mode) {
      highlightMode(data.op_mode);
    }

    // Firmware-Version
    if (data.fw_version) {
      document.getElementById('fwCurrentVersion').textContent = data.fw_version;
      const aboutVer = document.getElementById('fwVersionDisplayAbout');
      if (aboutVer) aboutVer.textContent = data.fw_version;
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

    // Timer window display (with temperature extension)
    if (data.timer_start_h != null && data.timer_end_h != null) {
      const pad2 = (n) => n.toString().padStart(2, '0');
      const sh = pad2(data.timer_start_h);
      const sm = pad2(data.timer_start_m);
      const eh = pad2(data.timer_end_h);
      const em = pad2(data.timer_end_m);

      const extMinutes = data.circulation_extension || 0;
      const el = document.getElementById('timerDisplayVal');

      if (extMinutes > 0) {
        // Extended end: timer start + effective runtime
        const effectiveH = Math.floor(data.effective_runtime / 60);
        const effectiveM = data.effective_runtime % 60;
        const extraH = Math.floor(extMinutes / 60);
        const extraM = extMinutes % 60;

        const extendedTotal = (data.timer_start_h * 60 + data.timer_start_m) + data.effective_runtime;
        const extHour = Math.floor(extendedTotal / 60) % 24;
        const extMin = extendedTotal % 60;

        el.innerHTML = sh + ':' + sm + '&rarr;' + pad2(extHour) + ':' + pad2(extMin) +
          ' <span style="color:var(--accent-solar);font-size:0.65rem;">+' + extraH + 'h' + (extraM > 0 ? extraM + 'm' : '') + '</span>';
      } else {
        el.innerHTML = sh + ':' + sm + '&rarr;' + eh + ':' + em;
      }
    }

    // ── Auth state ──
    if (data.authenticated !== undefined) {
      isAuthenticated = data.authenticated;
    }
    updateAuthUI();

    // ── Pool & Time Tab (read-only params from /api/status) ──
    const statusFields = [
      ['loopInterval', data.loop_interval],
      ['tempMaxPool', data.temp_max_pool],
      ['tempMinSolar', data.temp_min_solar],
      ['tempHysteresis', data.temp_hysteresis],
      ['tempCircThreshold', data.temp_circ_threshold],
      ['tempCircFactor', data.temp_circ_factor],
      ['tempCircMaxRuntime', data.temp_circ_max_runtime],
      ['timezone', data.timezone],
      ['timeLossGreen', data.time_loss_green_hours],
      ['timeLossRed', data.time_loss_red_hours],
      ['ntpServer', data.ntp_server],
    ];
    for (const [id, val] of statusFields) {
      if (val != null) {
        const el = document.getElementById(id);
        if (el) el.value = val;
      }
    }

    // Timer start/end fields on Pool tab
    if (data.timer_start_h != null) {
      const pad2 = (n) => n.toString().padStart(2, '0');
      const stEl = document.getElementById('timerStart');
      if (stEl) stEl.value = pad2(data.timer_start_h) + ':' + pad2(data.timer_start_m);
      const etEl = document.getElementById('timerEnd');
      if (etEl) etEl.value = pad2(data.timer_end_h) + ':' + pad2(data.timer_end_m);
    }

    // AP-Mode: WiFi-Tab anzeigen (nur einmalig — nicht bei jedem Poll erzwingen,
    // sonst überschreibt es die manuelle Tab-Navigation des Nutzers)
    if (data.ap_mode) {
      if (!hasAutoSwitchedToWifi) {
        switchTab('wifi');
        hasAutoSwitchedToWifi = true;
      }
    } else {
      hasAutoSwitchedToWifi = false;
    }

    // About Tab – system info
    if (data.uptime != null) {
      const aboutUptime = document.getElementById('aboutUptime');
      if (aboutUptime) {
        const d = Math.floor(data.uptime / 86400);
        const h = Math.floor((data.uptime % 86400) / 3600);
        const m = Math.floor((data.uptime % 3600) / 60);
        aboutUptime.textContent = d + 'd ' + h + 'h ' + m + 'm';
      }
    }
    if (data.free_heap != null) {
      const aboutHeap = document.getElementById('aboutHeap');
      if (aboutHeap) aboutHeap.textContent = (data.free_heap / 1024).toFixed(0) + ' KB';
    }
    if (data.local_ip) {
      const aboutIP = document.getElementById('aboutIP');
      if (aboutIP) aboutIP.textContent = data.local_ip;
    }
  } catch (e) {
    console.error('[pool] loadTelemetry error:', e);
  }
}

// ── Auth UI Gating ──

function updateAuthUI() {
  const loginBanner = document.getElementById('loginBanner');
  if (loginBanner) {
    loginBanner.style.display = isAuthenticated ? 'none' : 'flex';
  }

  // Dashboard: disable interactive controls
  const interactiveSelectors = [
    '.pump-switch-card',                // pump toggle switches
    '.mode-card',                       // mode buttons
    '#poolTemp',                        // max pool temp click
    '#solarTemp',                       // min solar temp click
  ];
  for (const sel of interactiveSelectors) {
    for (const el of document.querySelectorAll(sel)) {
      if (isAuthenticated) {
        el.style.cursor = el.dataset.origCursor || '';
        if (el.dataset.origOnclick) {
          el.setAttribute('onclick', el.dataset.origOnclick);
        }
        el.classList.remove('disabled');
      } else {
        if (!el.dataset.origCursor) el.dataset.origCursor = el.style.cursor;
        if (el.getAttribute('onclick')) {
          el.dataset.origOnclick = el.getAttribute('onclick');
          el.removeAttribute('onclick');
        }
        el.style.cursor = 'default';
        el.classList.add('disabled');
      }
    }
  }

  // Hide bottom tab bar when not authenticated — read-only dashboard only
  const tabBar = document.getElementById('tabBar');
  if (tabBar) tabBar.style.display = isAuthenticated ? '' : 'none';

  // Tab visibility — only ever force-hide when unauthenticated. Never force-show:
  // switchTab() is the sole authority for which tab is currently visible, otherwise
  // this would re-reveal a hidden tab on every poll and fight user navigation.
  const tabsToHide = ['tab-sensors'];
  for (const id of tabsToHide) {
    const el = document.getElementById(id);
    if (el && !isAuthenticated) el.style.display = 'none';
  }

  // Hide Sensors tab button
  const sensorsTabBtn = document.querySelector('.tab-bar-item[data-tab="sensors"]');
  if (sensorsTabBtn) sensorsTabBtn.style.display = isAuthenticated ? '' : 'none';

  // More menu: hide admin items (wifi, mqtt, system, logs)
  for (const item of document.querySelectorAll('.more-sheet-item')) {
    const text = item.textContent.trim().toLowerCase();
    if (text === 'wifi' || text === 'mqtt' || text.startsWith('system') || text.startsWith('🔒') || text.includes('logs')) {
      item.style.display = isAuthenticated ? '' : 'none';
    }
  }

  // Pool tab: disable all inputs, selects, buttons
  for (const el of document.querySelectorAll('#tab-pool input, #tab-pool select, #tab-pool button')) {
    el.disabled = !isAuthenticated;
    if (!isAuthenticated) {
      el.style.opacity = '0.6';
      el.style.cursor = 'not-allowed';
    } else {
      el.style.opacity = '';
      el.style.cursor = '';
    }
  }

  // Time tab: disable all inputs, selects, buttons
  for (const el of document.querySelectorAll('#tab-time input, #tab-time select, #tab-time button')) {
    el.disabled = !isAuthenticated;
    if (!isAuthenticated) {
      el.style.opacity = '0.6';
      el.style.cursor = 'not-allowed';
    } else {
      el.style.opacity = '';
      el.style.cursor = '';
    }
  }

  // System / WiFi / MQTT / Logs / Sensors tabs: fully hide when not authenticated. Never
  // force-show here — that previously used `''` (empty string), which falls back
  // to the CSS default `display:block`, making the tab visible again on every 2s
  // poll regardless of which tab switchTab() had actually activated (the reported
  // "always jumps back to WiFi Settings" bug).
  for (const id of ['tab-system', 'tab-wifi', 'tab-mqtt', 'tab-logs']) {
    const el = document.getElementById(id);
    if (el && !isAuthenticated) el.style.display = 'none';
  }

  // Sensors tab: disable radio buttons and hide save bar
  if (!isAuthenticated) {
    for (const el of document.querySelectorAll('#tab-sensors input[type="radio"]')) {
      el.disabled = true;
    }
    const saveBar = document.getElementById('sensorSaveBar');
    if (saveBar) saveBar.style.display = 'none';
  }
}

// ── Login Modal ──

function showLoginForm() {
  const modal = document.getElementById('loginModal');
  if (modal) modal.style.display = 'flex';
}

function closeLoginForm() {
  const modal = document.getElementById('loginModal');
  if (modal) modal.style.display = 'none';
  document.getElementById('loginError').style.display = 'none';
}

// Attach login form handler when DOM is ready
document.addEventListener('DOMContentLoaded', function() {
  const form = document.getElementById('loginForm');
  if (form) {
    form.addEventListener('submit', async (e) => {
      e.preventDefault();
      const pwd = document.getElementById('loginPassword').value;
      const err = document.getElementById('loginError');
      const res = await fetch('/api/login', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: 'password=' + encodeURIComponent(pwd)
      });
      if (res.status === 200) {
        window.location.reload();
      } else {
        err.style.display = 'block';
      }
    });
  }
});

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

  const res = await fetch('/api/config', {
    method: 'POST',
    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
    body: 'type=mqtt&host=' + encodeURIComponent(host) + '&port=' + portVal + '&username=' + encodeURIComponent(user) + '&password=' + encodeURIComponent(pass)
  });
  if (res.status === 200) alert('MQTT config saved!');
}

// ── Pump Switch UI Helper ──

function setPumpSwitch(pump, isOn) {
  const toggle = document.getElementById(pump + 'PumpToggle');
  const status = document.getElementById(pump + 'PumpStatus');
  if (toggle) {
    toggle.classList.toggle('on', isOn);
  }
  if (status) {
    status.textContent = isOn ? 'ON' : 'OFF';
    status.className = 'pump-switch-status ' + (isOn ? 'on' : 'off');
  }
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
    { id: 'tempCircThreshold',  name: 'Circ. Temp Threshold',   min: 0,   max: 40,   type: 'float' },
    { id: 'tempCircFactor',     name: 'Circ. Temp Factor',      min: 0,   max: 120,  type: 'int' },
    { id: 'tempCircMaxRuntime', name: 'Circ. Max Runtime',      min: 60,  max: 1440, type: 'int' },
    { id: 'btn1Min',    name: 'Button 1 Min ADC',    min: 0, max: 4095, type: 'int' },
    { id: 'btn1Max',    name: 'Button 1 Max ADC',    min: 0, max: 4095, type: 'int' },
    { id: 'btn2Min',    name: 'Button 2 Min ADC',    min: 0, max: 4095, type: 'int' },
    { id: 'btn2Max',    name: 'Button 2 Max ADC',    min: 0, max: 4095, type: 'int' },
    { id: 'btn3Min',    name: 'Button 3 Min ADC',    min: 0, max: 4095, type: 'int' },
    { id: 'btn3Max',    name: 'Button 3 Max ADC',    min: 0, max: 4095, type: 'int' },
    { id: 'btnNoPress', name: 'No-Press Threshold',  min: 0, max: 4095, type: 'int' },
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
  // Button ADC thresholds must form coherent, non-overlapping ranges
  const btnVal = (id) => parseInt(document.getElementById(id).value, 10);
  const b1Min = btnVal('btn1Min'), b1Max = btnVal('btn1Max');
  const b2Min = btnVal('btn2Min'), b2Max = btnVal('btn2Max');
  const b3Min = btnVal('btn3Min'), b3Max = btnVal('btn3Max');
  if (b1Min >= b1Max || b2Min >= b2Max || b3Min >= b3Max) {
    alert('Each button Min must be less than its Max.');
    return false;
  }
  if (b1Max > b2Min || b2Max > b3Min) {
    alert('Button ADC ranges must not overlap.');
    return false;
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
  const circThreshold = document.getElementById('tempCircThreshold').value;
  const circFactor = document.getElementById('tempCircFactor').value;
  const circMaxRuntime = document.getElementById('tempCircMaxRuntime').value;
  const tz = document.getElementById('timezone').value;
  const btn1Min = document.getElementById('btn1Min').value;
  const btn1Max = document.getElementById('btn1Max').value;
  const btn2Min = document.getElementById('btn2Min').value;
  const btn2Max = document.getElementById('btn2Max').value;
  const btn3Min = document.getElementById('btn3Min').value;
  const btn3Max = document.getElementById('btn3Max').value;
  const btnNoPress = document.getElementById('btnNoPress').value;

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
    body: 'type=settings&mode=' + mode + '&interval=' + interval + '&max_pool=' + maxPool + '&min_solar=' + minSolar + '&hysteresis=' + hysteresis + '&circ_threshold=' + circThreshold + '&circ_factor=' + circFactor + '&circ_max_runtime=' + circMaxRuntime + '&timezone=' + tz + '&green=' + green + '&red=' + red + timerParams() + '&ntp_server=' + ntpServer + '&btn1_min=' + btn1Min + '&btn1_max=' + btn1Max + '&btn2_min=' + btn2Min + '&btn2_max=' + btn2Max + '&btn3_min=' + btn3Min + '&btn3_max=' + btn3Max + '&btn_no_press=' + btnNoPress
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
  const currentPass = document.getElementById('currentPass').value;
  const newPass = document.getElementById('adminPass').value;
  const confirmPass = document.getElementById('adminPassConfirm').value;

  if (!currentPass) { alert('Please enter your current password.'); return; }
  if (!newPass || newPass.length < 4) { alert('New password must be at least 4 characters.'); return; }
  if (newPass !== confirmPass) { alert('New password and confirmation do not match.'); return; }

  const res = await fetch('/api/config', {
    method: 'POST',
    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
    body: 'type=password'
      + '&current_password=' + encodeURIComponent(currentPass)
      + '&password=' + encodeURIComponent(newPass)
      + '&password_confirm=' + encodeURIComponent(confirmPass)
  });
  if (res.status === 200) {
    alert('✓ Admin Password updated!');
    document.getElementById('currentPass').value = '';
    document.getElementById('adminPass').value = '';
    document.getElementById('adminPassConfirm').value = '';
  } else {
    const msg = await res.text();
    alert('✖ ' + (msg || 'Failed to update password'));
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

    document.getElementById('opMode').value = data.settings.op_mode;
    document.getElementById('loopInterval').value = data.settings.loop_interval;
    document.getElementById('tempMaxPool').value = data.settings.temp_max_pool;
    document.getElementById('tempMinSolar').value = data.settings.temp_min_solar;
    document.getElementById('tempHysteresis').value = data.settings.temp_hysteresis;
    document.getElementById('tempCircThreshold').value = data.settings.temp_circ_threshold;
    document.getElementById('tempCircFactor').value = data.settings.temp_circ_factor;
    document.getElementById('tempCircMaxRuntime').value = data.settings.temp_circ_max_runtime;
    document.getElementById('timezone').value = data.settings.timezone;
    document.getElementById('ntpServer').value = data.ntp.server;
    document.getElementById('timeLossGreen').value = data.settings.time_loss_green_hours;
    document.getElementById('timeLossRed').value = data.settings.time_loss_red_hours;
    document.getElementById('btn1Min').value = data.settings.btn1_min;
    document.getElementById('btn1Max').value = data.settings.btn1_max;
    document.getElementById('btn2Min').value = data.settings.btn2_min;
    document.getElementById('btn2Max').value = data.settings.btn2_max;
    document.getElementById('btn3Min').value = data.settings.btn3_min;
    document.getElementById('btn3Max').value = data.settings.btn3_max;
    document.getElementById('btnNoPress').value = data.settings.btn_no_press;
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

// ── Sensor Mapping ──

// Pending assignment: { solar: "28FF..."|null, pool: "28FF..."|null }
let pendingMapping = { solar: null, pool: null };
// Last loaded mapping for change detection
let loadedMapping = { solar: null, pool: null };

async function loadSensors() {
  try {
    const res = await fetch('/api/sensors');
    const data = await res.json();

    const solarAddr = data.mapping.solar || null;
    const poolAddr = data.mapping.pool || null;

    // Seed pending + loaded on first load
    if (loadedMapping.solar === null) {
      loadedMapping.solar = solarAddr;
      loadedMapping.pool = poolAddr;
      pendingMapping.solar = solarAddr;
      pendingMapping.pool = poolAddr;
    }

    // ── Build sensor list header ──
    const devices = data.detected || [];
    const list = document.getElementById('sensorList');
    if (devices.length === 0) {
      list.innerHTML = '<div style="padding: 1rem; text-align: center; color: var(--text-muted); font-size: 0.85rem;">⚠ No DS18B20 sensors detected on the bus.<br>Check wiring and pull-up resistor.</div>';
    } else {
      list.innerHTML = '<div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 0.5rem;">' +
        '<span style="font-size: 0.85rem; color: var(--text-muted);">' + devices.length + ' sensor(s) found on bus</span>' +
        '<button class="btn" onclick="loadSensors()" style="font-size: 0.75rem; padding: 0.25rem 0.6rem;">🔄 Refresh</button>' +
        '</div>' +
        '<div style="font-size: 0.75rem; color: var(--text-muted); margin-bottom: 0.75rem; line-height: 1.5;">' +
        devices.map(d => '<span style="font-family: monospace; background: rgba(255,255,255,0.05); padding: 0.1rem 0.4rem; border-radius: 3px;">' + d.address + '</span>').join(' ') +
        '</div>';
    }

    // ── Build solar radio group ──
    buildRadioGroup('solarRadioGroup', devices, solarAddr, poolAddr, 'solar');

    // ── Build pool radio group ──
    buildRadioGroup('poolRadioGroup', devices, solarAddr, poolAddr, 'pool');

    updateSensorSaveBar();
  } catch (e) {
    document.getElementById('sensorList').innerHTML =
      '<div style="padding: 1rem; text-align: center; color: var(--danger); font-size: 0.85rem;">Failed to load sensors: ' + e.message + '</div>';
  }
}

function buildRadioGroup(containerId, devices, solarAddr, poolAddr, role) {
  const container = document.getElementById(containerId);
  const otherRole = (role === 'solar') ? 'pool' : 'solar';
  const otherAddr = (role === 'solar') ? poolAddr : solarAddr;
  const currentAddr = (role === 'solar') ? pendingMapping.solar : pendingMapping.pool;

  let html = '';

  // "Not configured" option
  const noCfgChecked = (currentAddr === null) ? 'checked' : '';
  html += '<label style="display: flex; align-items: center; gap: 0.6rem; padding: 0.5rem 0.75rem; border-radius: 8px; border: 1px solid var(--panel-border); background: var(--panel-bg); cursor: pointer;">';
  html += '  <input type="radio" name="' + role + 'Sensor" value="" ' + noCfgChecked + ' onchange="selectSensor(\'' + role + '\', null)" style="accent-color: ' + (role === 'solar' ? 'var(--accent-solar)' : '#00b4d8') + ';">';
  html += '  <div style="flex: 1;">';
  html += '    <div style="font-size: 0.85rem; color: var(--text-muted);">— Not configured</div>';
  html += '  </div>';
  html += '</label>';

  // One radio per detected sensor
  devices.forEach(dev => {
    const addr = dev.address;
    const temp = dev.temperature != null ? dev.temperature.toFixed(1) + ' °C' : '--.- °C';
    const isAssignedToOther = (addr === otherAddr);
    const isCurrent = (addr === currentAddr);
    const checked = isCurrent ? 'checked' : '';
    const disabled = isAssignedToOther ? 'disabled' : '';
    const disabledNote = isAssignedToOther ? ' (already assigned to ' + otherRole + ')' : '';

    html += '<label style="display: flex; align-items: center; gap: 0.6rem; padding: 0.5rem 0.75rem; border-radius: 8px; border: 1px solid var(--panel-border); background: ' + (isCurrent ? 'rgba(0,229,255,0.08)' : 'var(--panel-bg)') + '; cursor: ' + (disabled ? 'not-allowed' : 'pointer') + '; opacity: ' + (disabled ? '0.5' : '1') + ';">';
    html += '  <input type="radio" name="' + role + 'Sensor" value="' + addr + '" ' + checked + ' ' + disabled + ' onchange="selectSensor(\'' + role + '\',\'' + addr + '\')" style="accent-color: ' + (role === 'solar' ? 'var(--accent-solar)' : '#00b4d8') + ';">';
    html += '  <div style="flex: 1; min-width: 0;">';
    html += '    <div style="font-family: monospace; font-size: 0.85rem;">' + addr + '</div>';
    html += '    <div style="font-size: 0.7rem; color: var(--text-muted);">' + temp + disabledNote + '</div>';
    html += '  </div>';
    html += '</label>';
  });

  container.innerHTML = html;
}

function selectSensor(role, address) {
  // If selecting the same address for both roles, deselect the other role
  const otherRole = (role === 'solar') ? 'pool' : 'solar';
  if (address !== null && pendingMapping[otherRole] === address) {
    pendingMapping[otherRole] = null;
  }
  pendingMapping[role] = address;

  // Re-render both radio groups so the other role reflects the updated disabled state
  loadSensors();
}

function updateSensorSaveBar() {
  const bar = document.getElementById('sensorSaveBar');
  const hasChanges = (pendingMapping.solar !== loadedMapping.solar) || (pendingMapping.pool !== loadedMapping.pool);
  bar.style.display = hasChanges ? 'block' : 'none';
}

async function saveSensorMapping() {
  const btn = document.getElementById('btnSaveMapping');
  const feedback = document.getElementById('sensorFeedback');
  btn.disabled = true;
  btn.textContent = '⏳ Saving...';

  const body = 'solar_addr=' + encodeURIComponent(pendingMapping.solar || '') +
    '&pool_addr=' + encodeURIComponent(pendingMapping.pool || '');

  try {
    const res = await fetch('/api/sensors/map', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: body
    });
    const data = await res.json();
    if (res.status === 200) {
      feedback.style.display = 'block';
      feedback.style.background = 'rgba(34, 197, 94, 0.15)';
      feedback.style.color = '#22c55e';
      feedback.textContent = '✓ Mapping saved! Rebooting in 3 seconds...';
      setTimeout(() => window.location.reload(), 3500);
    } else {
      feedback.style.display = 'block';
      feedback.style.background = 'rgba(239, 68, 68, 0.15)';
      feedback.style.color = '#ef4444';
      feedback.textContent = '✖ ' + (data.message || 'Failed to save mapping');
      btn.disabled = false;
      btn.textContent = '💾 Save Mapping & Reboot';
    }
  } catch (e) {
    feedback.style.display = 'block';
    feedback.style.background = 'rgba(239, 68, 68, 0.15)';
    feedback.style.color = '#ef4444';
    feedback.textContent = '✖ Network error: ' + e.message;
    btn.disabled = false;
    btn.textContent = '💾 Save Mapping & Reboot';
  }
}

// ── Log Console ──

var lastLogSeq = 0;
var lastLogBoot = 0;
var logLevelFilter = 'info';
// Generation token: bumped on every request, filter change and clear.
// Responses carrying an older token are discarded, so a slow in-flight
// poll cannot append stale/duplicate entries or overwrite lastLogSeq
// after a newer poll, filter switch or clear has happened.
var logReqToken = 0;
// Serializes polls: fetch() responses taking longer than the 2s tick must not
// start a second concurrent poll (whose response would bump the token and
// discard the first one — leaving the console stuck until the next clear).
var logPollInFlight = false;

function escapeHtml(s) {
  return String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;');
}

function loadLogs() {
  // Only poll while the Logs tab is actually visible: the unconditional 2s
  // timer used to keep appending DOM nodes (and fetching) in hidden tabs,
  // growing the document by tens of thousands of nodes per day.
  if (document.visibilityState !== 'visible') return;
  var logTab = document.getElementById('tab-logs');
  if (!logTab || logTab.style.display === 'none') return;

  // Never overlap polls: a slow response would otherwise be superseded by the
  // next tick's request (token bump) and discarded, stalling the console until
  // a clear or filter change. The next tick resumes after this one settles.
  if (logPollInFlight) return;

  var wasAtBottom, consoleEl = document.getElementById('logConsole');
  if (!consoleEl) return;
  wasAtBottom = consoleEl.scrollTop + consoleEl.clientHeight >= consoleEl.scrollHeight - 40;
  var token = ++logReqToken;
  logPollInFlight = true;
  // boot = epoch of the cursor: after a reboot the server forces a full dump
  // (entries 1..N) even when the new seq is already past our stored cursor.
  fetch('/api/logs?since=' + lastLogSeq + '&boot=' + lastLogBoot + '&count=200&level=' + logLevelFilter)
    .then(function(res) { return res.json(); })
    .then(function(data) {
      if (token !== logReqToken) return;  // superseded by a newer poll/filter/clear
      var empty = document.getElementById('logConsoleEmpty');
      // Boot change: the server re-sent the whole new-boot ring, so the old
      // pre-reboot lines are stale — drop them instead of appending on top.
      if (data.boot !== lastLogBoot) {
        consoleEl.textContent = '';
      }
      if (!data.entries || data.entries.length === 0) {
        if (!consoleEl.hasChildNodes()) empty.style.display = 'block';
        return;
      }
      empty.style.display = 'none';
      data.entries.forEach(function(entry) {
        var line = document.createElement('div');
        line.className = 'log-entry log-' + entry.level;
        line.textContent = entry.msg;
        consoleEl.appendChild(line);
      });
      // Evict oldest entries beyond the client-side cap so an always-open
      // dashboard cannot grow the log DOM without bound.
      while (consoleEl.childNodes.length > 500) {
        consoleEl.removeChild(consoleEl.firstChild);
      }
      lastLogSeq = data.next;
      lastLogBoot = data.boot;
      if (wasAtBottom && data.entries.length > 0) {
        consoleEl.scrollTop = consoleEl.scrollHeight;
      }
    })
    .catch(function() { /* silent */ })
    .finally(function() {
      logPollInFlight = false;
    });
}

function clearLogs() {
  logReqToken++;  // invalidate any in-flight poll — it must not repopulate the console
  fetch('/api/logs/clear', { method: 'POST' }).then(function() {
    var c = document.getElementById('logConsole');
    if (c) c.textContent = '';
    var e = document.getElementById('logConsoleEmpty');
    if (e) e.style.display = 'block';
    lastLogSeq = 0;
  });
}

document.addEventListener('DOMContentLoaded', function() {
  document.querySelectorAll('.log-chip').forEach(function(chip) {
    chip.addEventListener('click', function() {
      logReqToken++;  // discard in-flight responses from the previous filter
      document.querySelectorAll('.log-chip').forEach(function(c) { c.classList.remove('active'); });
      this.classList.add('active');
      logLevelFilter = this.dataset.level;
      lastLogSeq = 0;
      var c = document.getElementById('logConsole');
      if (c) c.textContent = '';
      var e = document.getElementById('logConsoleEmpty');
      if (e) e.style.display = 'none';
      loadLogs();
    });
  });
});

var _origUpdateAuthUI = (typeof updateAuthUI === 'function') ? updateAuthUI : function(){};
updateAuthUI = function() {
  _origUpdateAuthUI();
  var clearBtn = document.getElementById('btnClearLogs');
  if (clearBtn) clearBtn.style.display = isAuthenticated ? 'inline-block' : 'none';
};

// ── Init ──

setInterval(loadTelemetry, 2000);
setInterval(loadLogs, 2000);

window.onload = function() {
  loadTelemetry();
  loadConfig();
  loadSensors();
};
