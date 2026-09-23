/**
 * P10 IoT Controller - Cloud Edition
 */

const API_URL = 'https://digital-clock-production.up.railway.app';

const state = {
  token: localStorage.getItem('p10_token') || null,
  user: JSON.parse(localStorage.getItem('p10_user') || 'null'),
  devices: [],
  activeDevice: null,
  connMode: localStorage.getItem('p10_conn_mode') || 'cloud',
  text1: "HALLO",
  anim: "scroll_left",
  speed: 5,
  clockDuration: 10,
  textDuration: 15,
  displayMode: "cycle",
  brightness: 20,
  autoDimming: false,
  power: true,
  simCurrentView: "clock",
  simTimer: null
};

const el = {
  authScreen: document.getElementById('authScreen'),
  mainApp: document.getElementById('mainApp'),
  authForm: document.getElementById('authForm'),
  authEmail: document.getElementById('authEmail'),
  authPassword: document.getElementById('authPassword'),
  authName: document.getElementById('authName'),
  authSubmit: document.getElementById('authSubmit'),
  authToggle: document.getElementById('authToggle'),
  authError: document.getElementById('authError'),
  registerFields: document.getElementById('registerFields'),
  btnLogout: document.getElementById('btnLogout'),
  userName: document.getElementById('userName'),
  navItems: document.querySelectorAll('.nav-item'),
  tabContents: document.querySelectorAll('.tab-content'),
  p10Matrix: document.getElementById('p10Matrix'),
  matrixClockView: document.getElementById('matrixClockView'),
  matrixMarqueeView: document.getElementById('matrixMarqueeView'),
  simTimeDisplay: document.getElementById('simHours'),
  simMinDisplay: document.getElementById('simMin'),
  simDateDisplay: document.getElementById('simDate'),
  simMarqueeContent: document.getElementById('simMarqueeContent'),
  simStatusInfo: document.getElementById('simStatusInfo'),
  simModeLabel: document.getElementById('simModeLabel'),
  btnToggleSimMode: document.getElementById('btnToggleSimMode'),
  badgeText: document.getElementById('badgeText'),
  btnPowerToggle: document.getElementById('btnPowerToggle'),
  powerStatus: document.getElementById('powerStatus'),
  powerStatusCard: document.getElementById('powerStatusCard'),
  btnThemeToggle: document.getElementById('btnThemeToggle'),
  themeLabel: document.getElementById('themeLabel'),
  inputText1: document.getElementById('inputText1'),
  charCount1: document.getElementById('charCount1'),
  presetChips: document.querySelectorAll('.chip'),
  btnClearInputs: document.querySelectorAll('.btn-clear'),
  animRadios: document.querySelectorAll('input[name="animEffect"]'),
  animOptions: document.querySelectorAll('.anim-option'),
  speedSlider: document.getElementById('speedSlider'),
  speedValueDisplay: document.getElementById('speedValueDisplay'),
  clockDuration: document.getElementById('clockDuration'),
  clockDurationDisplay: document.getElementById('clockDurationDisplay'),
  textDuration: document.getElementById('textDuration'),
  textDurationDisplay: document.getElementById('textDurationDisplay'),
  segBtns: document.querySelectorAll('.seg-btn'),
  brightnessSlider: document.getElementById('brightnessSlider'),
  brightnessValueDisplay: document.getElementById('brightnessValueDisplay'),
  toggleAutoDimming: document.getElementById('toggleAutoDimming'),
  btnSendToESP: document.getElementById('btnSendToESP'),
  deviceSelect: document.getElementById('deviceSelect'),
  btnAddDevice: document.getElementById('btnAddDevice'),
  btnDeleteDevice: document.getElementById('btnDeleteDevice'),
  btnRefreshDevices: document.getElementById('btnRefreshDevices'),
  deviceUidInput: document.getElementById('deviceUidInput'),
  deviceNameInput: document.getElementById('deviceNameInput'),
  addDeviceModal: document.getElementById('addDeviceModal'),
  toastContainer: document.getElementById('toastContainer'),
  infoDeviceName: document.getElementById('infoDeviceName'),
  infoDeviceUid: document.getElementById('infoDeviceUid'),
  infoDeviceStatus: document.getElementById('infoDeviceStatus'),
  infoLastSeen: document.getElementById('infoLastSeen'),
  infoConnMode: document.getElementById('infoConnMode'),
  btnConnMode: document.getElementById('btnConnMode'),
  connModeLabel: document.getElementById('connModeLabel'),
  connModeDot: document.getElementById('connModeDot'),
  localHostInput: document.getElementById('localHostInput'),
  btnSaveLocalHost: document.getElementById('btnSaveLocalHost'),
  hamburgerBtn: document.getElementById('hamburgerBtn'),
  sidebarOverlay: document.getElementById('sidebarOverlay'),
  appSidebar: document.getElementById('appSidebar'),
  mobileHeader: document.getElementById('mobileHeader')
};

// ==================== THEME (Tempora Nova: default dark, light optional) ====================
function initTheme() {
  const saved = localStorage.getItem('tempora_theme');
  const theme = saved || 'dark';
  document.documentElement.setAttribute('data-theme', theme);
  updateThemeLabel(theme);
}
function updateThemeLabel(theme) {
  if (el.themeLabel) el.themeLabel.textContent = theme === 'dark' ? 'Mode Gelap' : 'Mode Terang';
}
function toggleTheme() {
  const cur = document.documentElement.getAttribute('data-theme') || 'dark';
  const next = cur === 'dark' ? 'light' : 'dark';
  document.documentElement.setAttribute('data-theme', next);
  localStorage.setItem('tempora_theme', next);
  updateThemeLabel(next);
}

// ==================== CONNECTION MODE (Cloud / Lokal) ====================
function toggleConnMode() {
  state.connMode = state.connMode === 'local' ? 'cloud' : 'local';
  localStorage.setItem('p10_conn_mode', state.connMode);
  updateConnModeUI();
}

function updateConnModeUI() {
  const isLocal = state.connMode === 'local';
  if (el.connModeLabel) el.connModeLabel.textContent = isLocal ? 'Mode: Lokal (Gratis)' : 'Mode: Cloud (Premium)';
  if (el.connModeDot) el.connModeDot.classList.toggle('local', isLocal);
  if (el.infoConnMode) el.infoConnMode.textContent = isLocal ? 'Lokal (Gratis)' : 'Cloud (Premium)';
}

function getLocalBase() {
  if (!state.activeDevice || !state.activeDevice.local_host) return null;
  let host = String(state.activeDevice.local_host).trim().replace(/^https?:\/\//i, '').replace(/\/+$/, '');
  if (!host) return null;
  return `http://${host}`;
}

function buildLocalPayload(overrides = {}) {
  const p = { ...buildPayload(), ...overrides };
  return {
    text1: p.text1,
    anim: p.anim,
    speed_ms: Math.max(15, 120 - (p.speed * 10)),
    clock_duration: p.clock_duration,
    text_duration: p.text_duration,
    mode: p.display_mode,
    brightness_pwm: Math.round((p.brightness / 100) * 255),
    power: p.power !== false,
    format_24h: true,
    show_seconds: true
  };
}

// ==================== SIDEBAR TOGGLE (MOBILE) ====================
function openSidebar() {
  if (el.appSidebar) el.appSidebar.classList.add('open');
  if (el.sidebarOverlay) el.sidebarOverlay.classList.add('active');
  if (el.hamburgerBtn) el.hamburgerBtn.setAttribute('aria-expanded', 'true');
  document.body.style.overflow = 'hidden';
}

function closeSidebar() {
  if (el.appSidebar) el.appSidebar.classList.remove('open');
  if (el.sidebarOverlay) el.sidebarOverlay.classList.remove('active');
  if (el.hamburgerBtn) el.hamburgerBtn.setAttribute('aria-expanded', 'false');
  document.body.style.overflow = '';
}

function toggleSidebar() {
  if (el.appSidebar && el.appSidebar.classList.contains('open')) {
    closeSidebar();
  } else {
    openSidebar();
  }
}

// ==================== AUTH ====================
document.addEventListener('DOMContentLoaded', () => {
  initTheme();
  updateConnModeUI();
  if (state.token && state.user) {
    showMainApp();
  } else {
    showAuthScreen();
  }
  setupAuthListeners();
  setupNavigationTabs();
  setupEventListeners();
  startClockTicker();
  updateCharCounts();
  updateBadgeLabels();
});

function showAuthScreen() {
  if (el.authScreen) el.authScreen.classList.remove('hidden');
  if (el.mainApp) el.mainApp.classList.add('hidden');
}

function showMainApp() {
  if (el.authScreen) el.authScreen.classList.add('hidden');
  if (el.mainApp) el.mainApp.classList.remove('hidden');
  if (el.userName) el.userName.textContent = state.user.name || state.user.email;
  loadDevices();
  startSimulatorCycle();
  updateSimulatorUI();
  startStatusPolling();
}

let statusPollTimer = null;
function startStatusPolling() {
  if (statusPollTimer) clearInterval(statusPollTimer);
  statusPollTimer = setInterval(async () => {
    if (!state.activeDevice) return;
    try {
      const res = await apiFetch(`/api/devices/${state.activeDevice.id}`);
      if (!res.ok) return;
      const device = await res.json();
      const idx = state.devices.findIndex(d => d.id === device.id);
      if (idx >= 0) {
        state.devices[idx].online = device.online;
        state.devices[idx].last_seen = device.last_seen;
      }
      if (el.badgeText) {
        const status = device.online ? 'Online' : 'Offline';
        el.badgeText.textContent = `${state.activeDevice.name} \u2022 ${status}`;
      }
      renderDeviceList();
    } catch (e) {}
  }, 5000);
}

function setupAuthListeners() {
  let isRegister = false;

  if (el.authToggle) {
    el.authToggle.addEventListener('click', () => {
      isRegister = !isRegister;
      if (el.authSubmit) el.authSubmit.textContent = isRegister ? 'Daftar' : 'Masuk';
      if (el.authToggle) el.authToggle.textContent = isRegister ? 'Sudah punya akun? Masuk' : 'Belum punya akun? Daftar';
      if (el.registerFields) el.registerFields.classList.toggle('hidden', !isRegister);
      if (el.authError) el.authError.textContent = '';
    });
  }

  if (el.authForm) {
    el.authForm.addEventListener('submit', async (e) => {
      e.preventDefault();
      const email = el.authEmail.value.trim();
      const password = el.authPassword.value;
      const name = el.authName ? el.authName.value.trim() : '';

      if (!email || !password) {
        if (el.authError) el.authError.textContent = 'Email dan password wajib diisi';
        return;
      }

      const endpoint = isRegister ? '/api/auth/register' : '/api/auth/login';
      const body = isRegister ? { email, password, name } : { email, password };

      try {
        if (el.authSubmit) el.authSubmit.disabled = true;
        if (el.authError) el.authError.textContent = '';

        const res = await fetch(`${API_URL}${endpoint}`, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(body)
        });

        const data = await res.json();
        if (!res.ok) {
          if (el.authError) el.authError.textContent = data.error || 'Terjadi kesalahan';
          return;
        }

        state.token = data.token;
        state.user = data.user;
        localStorage.setItem('p10_token', data.token);
        localStorage.setItem('p10_user', JSON.stringify(data.user));
        showMainApp();
      } catch (err) {
        if (el.authError) el.authError.textContent = 'Gagal terhubung ke server';
      } finally {
        if (el.authSubmit) el.authSubmit.disabled = false;
      }
    });
  }

  if (el.btnLogout) {
    el.btnLogout.addEventListener('click', () => {
      state.token = null;
      state.user = null;
      state.devices = [];
      state.activeDevice = null;
      localStorage.removeItem('p10_token');
      localStorage.removeItem('p10_user');
      showAuthScreen();
    });
  }
}

// ==================== API HELPERS ====================
async function apiFetch(path, options = {}) {
  const res = await fetch(`${API_URL}${path}`, {
    ...options,
    headers: {
      'Content-Type': 'application/json',
      'Authorization': `Bearer ${state.token}`,
      ...options.headers
    }
  });

  if (res.status === 401) {
    state.token = null;
    state.user = null;
    localStorage.removeItem('p10_token');
    localStorage.removeItem('p10_user');
    showAuthScreen();
    throw new Error('Session expired');
  }

  return res;
}

// ==================== DEVICES ====================
async function loadDevices() {
  try {
    const res = await apiFetch('/api/devices');
    if (!res.ok) return;
    state.devices = await res.json();
    renderDeviceList();

    if (state.devices.length > 0 && !state.activeDevice) {
      selectDevice(state.devices[0].id);
    }
  } catch (err) {
    console.error('Load devices error:', err);
  }
}

function renderDeviceList() {
  if (!el.deviceSelect) return;
  el.deviceSelect.innerHTML = '';

  if (state.devices.length === 0) {
    el.deviceSelect.innerHTML = '<option value="">Belum ada perangkat</option>';
    return;
  }

  state.devices.forEach(d => {
    const opt = document.createElement('option');
    opt.value = d.id;
    opt.textContent = `${d.name} (${d.online ? 'Online' : 'Offline'})`;
    if (state.activeDevice && state.activeDevice.id === d.id) opt.selected = true;
    el.deviceSelect.appendChild(opt);
  });
}

function selectDevice(id) {
  const device = state.devices.find(d => d.id === parseInt(id));
  if (!device) return;
  state.activeDevice = device;

  state.text1 = device.text1 || "SELAMAT DATANG";
  state.anim = device.anim || "scroll_left";
  state.speed = device.speed || 5;
  state.clockDuration = device.clock_duration || 10;
  state.textDuration = device.text_duration || 15;
  state.displayMode = device.display_mode || "cycle";
  state.brightness = device.brightness || 20;
  state.autoDimming = device.auto_dimming || false;
  state.power = device.power !== false;

  if (el.inputText1) el.inputText1.value = state.text1;
  if (el.speedSlider) el.speedSlider.value = state.speed;
  if (el.clockDuration) el.clockDuration.value = state.clockDuration;
  if (el.textDuration) el.textDuration.value = state.textDuration;
  if (el.brightnessSlider) el.brightnessSlider.value = state.brightness;
  if (el.toggleAutoDimming) el.toggleAutoDimming.checked = state.autoDimming;

  el.animRadios.forEach(r => {
    const wrapper = r.closest('.anim-option');
    r.checked = r.value === state.anim;
    if (wrapper) wrapper.classList.toggle('active', r.value === state.anim);
  });

  el.segBtns.forEach(b => b.classList.toggle('active', b.dataset.mode === state.displayMode));

  if (el.badgeText) {
    const status = device.online ? 'Online' : 'Offline';
    el.badgeText.textContent = `${device.name} \u2022 ${status}`;
  }

  updateBadgeLabels();
  updateSimulatorUI();
  updatePowerUI();
  startSimulatorCycle();
  updateDeviceTab();
  closeSidebar();
}

async function addDevice() {
  const uid = el.deviceUidInput ? el.deviceUidInput.value.trim() : '';
  const name = el.deviceNameInput ? el.deviceNameInput.value.trim() : 'P10 Panel';
  if (!uid) {
    showToast('Masukkan Device UID', 'error');
    return;
  }

  try {
    const res = await apiFetch('/api/devices', {
      method: 'POST',
      body: JSON.stringify({ device_uid: uid, name })
    });

    if (!res.ok) {
      const data = await res.json();
      showToast(data.error || 'Gagal menambah perangkat', 'error');
      return;
    }

    showToast('Perangkat berhasil ditambahkan!', 'success');
    closeAddDeviceModal();
    await loadDevices();
  } catch (err) {
    showToast('Gagal terhubung ke server', 'error');
  }
}

function openAddDeviceModal() {
  if (el.addDeviceModal) el.addDeviceModal.classList.remove('hidden');
}

function closeAddDeviceModal() {
  if (el.addDeviceModal) el.addDeviceModal.classList.add('hidden');
  if (el.deviceUidInput) el.deviceUidInput.value = '';
  if (el.deviceNameInput) el.deviceNameInput.value = '';
}

async function deleteDevice() {
  if (!state.activeDevice) {
    showToast('Pilih perangkat terlebih dahulu', 'error');
    return;
  }
  if (!confirm(`Hapus perangkat "${state.activeDevice.name}"?`)) return;

  try {
    const res = await apiFetch(`/api/devices/${state.activeDevice.id}`, { method: 'DELETE' });
    if (!res.ok) {
      showToast('Gagal menghapus perangkat', 'error');
      return;
    }
    showToast('Perangkat dihapus!', 'success');
    state.activeDevice = null;
    await loadDevices();
  } catch (err) {
    showToast('Gagal terhubung ke server', 'error');
  }
}

// ==================== NAVIGATION ====================
function setupNavigationTabs() {
  el.navItems.forEach(btn => {
    btn.addEventListener('click', () => {
      const tab = btn.dataset.tab;
      el.navItems.forEach(i => i.classList.remove('active'));
      btn.classList.add('active');
      el.tabContents.forEach(p => {
        p.classList.remove('active');
        if (p.id === `tabContent${capitalize(tab)}`) p.classList.add('active');
      });
      window.scrollTo({ top: 0, behavior: 'smooth' });
      closeSidebar();
    });
  });
}

function capitalize(s) { return s ? s.charAt(0).toUpperCase() + s.slice(1) : ''; }

// ==================== CLOCK ====================
function startClockTicker() {
  updateTimeDisplays();
  setInterval(updateTimeDisplays, 1000);
}

function getFormattedTime(date = new Date()) {
  return {
    hours: String(date.getHours()).padStart(2, '0'),
    minutes: String(date.getMinutes()).padStart(2, '0'),
    seconds: String(date.getSeconds()).padStart(2, '0')
  };
}

function getFormattedDate(date = new Date()) {
  const days = ['Minggu', 'Senin', 'Selasa', 'Rabu', 'Kamis', 'Jumat', 'Sabtu'];
  return days[date.getDay()];
}

function updateTimeDisplays() {
  const { hours, minutes } = getFormattedTime();
  if (state.simCurrentView === 'clock') {
    if (el.simTimeDisplay) el.simTimeDisplay.textContent = hours;
    if (el.simMinDisplay) el.simMinDisplay.textContent = minutes;
    if (el.simDateDisplay) el.simDateDisplay.textContent = getFormattedDate();
  }
}

// ==================== SIMULATOR ====================
function updateSimulatorUI() {
  if (!el.simMarqueeContent) return;
  el.simMarqueeContent.textContent = state.text1 || "P10 ESP8266";
  el.simMarqueeContent.className = 'marquee-text';

  const dur = Math.max(2.5, 22 - (state.speed * 2.0));
  el.simMarqueeContent.style.animationDuration = `${dur}s`;

  if (state.anim === 'scroll_left' || state.anim === 'scroll_right') {
    if (el.matrixMarqueeView) el.matrixMarqueeView.classList.add('is-scroll');
    el.simMarqueeContent.classList.add(state.anim === 'scroll_left' ? 'scroll-left' : 'scroll-right');
  } else {
    if (el.matrixMarqueeView) el.matrixMarqueeView.classList.remove('is-scroll');
  }

  if (el.simStatusInfo) {
    const names = { scroll_left: 'Scroll Kiri', scroll_right: 'Scroll Kanan', static: 'Diam' };
    el.simStatusInfo.textContent = `${names[state.anim] || 'Scroll'} \u2022 Speed Lv ${state.speed} \u2022 Terang ${state.brightness}%`;
  }
}

function startSimulatorCycle() {
  if (state.simTimer) clearTimeout(state.simTimer);
  if (state.displayMode === 'clock_only') { switchSimView('clock'); return; }
  if (state.displayMode === 'text_only') { switchSimView('marquee'); return; }

  let isClock = true;
  switchSimView('clock');

  function scheduleNext() {
    const delay = (isClock ? state.clockDuration : state.textDuration) * 1000;
    state.simTimer = setTimeout(() => {
      isClock = !isClock;
      if (!isClock) { updateSimulatorUI(); switchSimView('marquee'); }
      else { switchSimView('clock'); }
      scheduleNext();
    }, delay);
  }
  scheduleNext();
}

function switchSimView(view) {
  state.simCurrentView = view;
  if (view === 'clock') {
    if (el.matrixClockView) el.matrixClockView.classList.remove('hidden');
    if (el.matrixMarqueeView) el.matrixMarqueeView.classList.add('hidden');
    if (el.simModeLabel) el.simModeLabel.textContent = "Mode: Jam";
    updateTimeDisplays();
  } else {
    if (el.matrixClockView) el.matrixClockView.classList.add('hidden');
    if (el.matrixMarqueeView) el.matrixMarqueeView.classList.remove('hidden');
    if (el.simModeLabel) el.simModeLabel.textContent = "Mode: Teks";
    updateSimulatorUI();
  }
}

// ==================== EVENT LISTENERS ====================
function setupEventListeners() {
  // Sidebar toggle (mobile)
  if (el.hamburgerBtn) el.hamburgerBtn.addEventListener('click', toggleSidebar);
  if (el.sidebarOverlay) el.sidebarOverlay.addEventListener('click', closeSidebar);

  if (el.inputText1) {
    el.inputText1.addEventListener('input', (e) => {
      state.text1 = e.target.value;
      updateCharCounts();
      updateSimulatorUI();
    });
  }

  el.btnClearInputs.forEach(btn => {
    btn.addEventListener('click', () => {
      const input = document.getElementById(btn.dataset.target);
      if (input) { input.value = ''; input.dispatchEvent(new Event('input')); }
    });
  });

  el.presetChips.forEach(chip => {
    chip.addEventListener('click', () => {
      if (el.inputText1) {
        el.inputText1.value = chip.dataset.text;
        el.inputText1.dispatchEvent(new Event('input'));
        showToast(`Template "${chip.textContent}" diterapkan!`, 'info');
      }
    });
  });

  el.animRadios.forEach(radio => {
    radio.addEventListener('change', (e) => {
      if (e.target.checked) {
        state.anim = e.target.value;
        el.animOptions.forEach(o => o.classList.remove('active'));
        const wrapper = e.target.closest('.anim-option');
        if (wrapper) wrapper.classList.add('active');
        updateSimulatorUI();
      }
    });
  });

  if (el.speedSlider) {
    el.speedSlider.addEventListener('input', (e) => {
      state.speed = parseInt(e.target.value, 10);
      updateBadgeLabels();
      updateSimulatorUI();
    });
  }

  if (el.clockDuration) {
    el.clockDuration.addEventListener('input', (e) => {
      state.clockDuration = parseInt(e.target.value, 10);
      updateBadgeLabels();
      startSimulatorCycle();
    });
  }

  if (el.textDuration) {
    el.textDuration.addEventListener('input', (e) => {
      state.textDuration = parseInt(e.target.value, 10);
      updateBadgeLabels();
      startSimulatorCycle();
    });
  }

  el.segBtns.forEach(btn => {
    btn.addEventListener('click', () => {
      el.segBtns.forEach(b => b.classList.remove('active'));
      btn.classList.add('active');
      state.displayMode = btn.dataset.mode;
      startSimulatorCycle();
    });
  });

  if (el.brightnessSlider) {
    el.brightnessSlider.addEventListener('input', (e) => {
      state.brightness = parseInt(e.target.value, 10);
      updateBadgeLabels();
    });
  }

  if (el.toggleAutoDimming) {
    el.toggleAutoDimming.addEventListener('change', (e) => {
      state.autoDimming = e.target.checked;
    });
  }

  if (el.btnToggleSimMode) {
    el.btnToggleSimMode.addEventListener('click', () => {
      switchSimView(state.simCurrentView === 'clock' ? 'marquee' : 'clock');
    });
  }

  if (el.btnThemeToggle) el.btnThemeToggle.addEventListener('click', toggleTheme);
  if (el.btnConnMode) el.btnConnMode.addEventListener('click', toggleConnMode);
  if (el.btnPowerToggle) el.btnPowerToggle.addEventListener('click', () => togglePower());

  if (el.btnSendToESP) el.btnSendToESP.addEventListener('click', () => sendFullConfig());

  if (el.btnSaveLocalHost) el.btnSaveLocalHost.addEventListener('click', saveLocalHost);

  if (el.deviceSelect) {
    el.deviceSelect.addEventListener('change', (e) => {
      selectDevice(e.target.value);
    });
  }

  if (el.btnAddDevice) el.btnAddDevice.addEventListener('click', openAddDeviceModal);
  if (el.btnDeleteDevice) el.btnDeleteDevice.addEventListener('click', deleteDevice);
  if (el.btnRefreshDevices) el.btnRefreshDevices.addEventListener('click', loadDevices);

  if (el.addDeviceModal) {
    el.addDeviceModal.addEventListener('click', (e) => {
      if (e.target === el.addDeviceModal) closeAddDeviceModal();
    });
  }

  document.addEventListener('click', (e) => {
    if (e.target.id === 'btnConfirmAddDevice') addDevice();
    if (e.target.id === 'btnCancelAddDevice') closeAddDeviceModal();
  });
}

function updateCharCounts() {
  if (el.charCount1 && el.inputText1) el.charCount1.textContent = `${el.inputText1.value.length}/150`;
}

function updateBadgeLabels() {
  if (el.speedValueDisplay) el.speedValueDisplay.textContent = `Level ${state.speed}`;
  if (el.clockDurationDisplay) el.clockDurationDisplay.textContent = `${state.clockDuration}s`;
  if (el.textDurationDisplay) el.textDurationDisplay.textContent = `${state.textDuration}s`;
  if (el.brightnessValueDisplay) el.brightnessValueDisplay.textContent = `${state.brightness}%`;
}

function updateDeviceTab() {
  const device = state.activeDevice;
  if (!device) return;

  if (el.infoDeviceName) el.infoDeviceName.textContent = device.name || '-';
  if (el.infoDeviceUid) el.infoDeviceUid.textContent = device.device_uid || '-';

  if (el.infoConnMode) {
    const isLocal = state.connMode === 'local';
    const hasHost = !!(device.local_host && String(device.local_host).trim());
    let modeText = isLocal ? 'Lokal (Gratis)' : 'Cloud (Premium)';
    if (isLocal && hasHost) modeText += ` \u2192 ${device.local_host}`;
    else if (isLocal && !hasHost) modeText += ' \u2014 isi IP di bawah';
    el.infoConnMode.textContent = modeText;
  }

  if (el.localHostInput) el.localHostInput.value = device.local_host || '';

  if (el.infoDeviceStatus) {
    if (device.online) {
      el.infoDeviceStatus.innerHTML = '<span class="status-dot online"></span> Sedang Online';
    } else {
      el.infoDeviceStatus.innerHTML = '<span class="status-dot offline"></span> Offline';
    }
  }

  if (el.infoLastSeen) {
    if (device.online) {
      el.infoLastSeen.textContent = 'Sedang terhubung';
    } else if (device.last_seen) {
      const d = new Date(device.last_seen);
      const day = d.toLocaleDateString('id-ID', { day: 'numeric', month: 'short', year: 'numeric' });
      const time = d.toLocaleTimeString('id-ID', { hour: '2-digit', minute: '2-digit' });
      el.infoLastSeen.textContent = `${day}, ${time}`;
    } else {
      el.infoLastSeen.textContent = '-';
    }
  }
}

// ==================== CLOUD API ====================
function buildPayload() {
  return {
    text1: state.text1,
    anim: state.anim,
    speed: state.speed,
    clock_duration: state.clockDuration,
    text_duration: state.textDuration,
    display_mode: state.displayMode,
    brightness: state.brightness,
    auto_dimming: state.autoDimming,
    power: state.power
  };
}

async function saveLocalHost() {
  if (!state.activeDevice) {
    showToast('Pilih perangkat terlebih dahulu', 'error');
    return;
  }
  const host = el.localHostInput ? el.localHostInput.value.trim() : '';
  try {
    const res = await apiFetch(`/api/devices/${state.activeDevice.id}`, {
      method: 'PUT',
      body: JSON.stringify({ local_host: host })
    });
    if (!res.ok) {
      showToast('Gagal menyimpan IP lokal', 'error');
      return;
    }
    const data = await res.json();
    const idx = state.devices.findIndex(d => d.id === state.activeDevice.id);
    if (idx >= 0) {
      state.devices[idx].local_host = data.local_host;
      state.activeDevice.local_host = data.local_host;
    }
    updateDeviceTab();
    showToast(host ? 'IP lokal tersimpan' : 'IP lokal dihapus', 'success');
  } catch (err) {
    showToast('Gagal terhubung ke server', 'error');
  }
}

async function sendFullConfig() {
  if (!state.activeDevice) {
    showToast('Pilih perangkat terlebih dahulu', 'error');
    return;
  }

  if (state.connMode === 'local') {
    const base = getLocalBase();
    if (!base) {
      showToast('Isi IP Lokal di tab Perangkat', 'error');
      return;
    }
    showToast('Mengirim langsung ke panel (lokal)...', 'info');
    try {
      const res = await fetch(`${base}/api/settings`, {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(buildLocalPayload())
      });
      if (!res.ok) throw new Error('HTTP ' + res.status);
      showToast('Berhasil dikirim ke panel!', 'success');
    } catch (err) {
      showToast('Gagal ke panel lokal. Cek IP & WiFi sama.', 'error');
    }
    return;
  }

  const payload = buildPayload();
  showToast('Mengirim ke Panel P10...', 'info');

  try {
    const res = await apiFetch(`/api/devices/${state.activeDevice.id}/settings`, {
      method: 'PUT',
      body: JSON.stringify(payload)
    });

    if (!res.ok) {
      const data = await res.json();
      showToast(data.error || 'Gagal mengirim', 'error');
      return;
    }

    showToast('Pengaturan berhasil dikirim!', 'success');

    const updated = await res.json();
    const idx = state.devices.findIndex(d => d.id === state.activeDevice.id);
    if (idx >= 0) {
      Object.assign(state.devices[idx], payload);
    }
  } catch (err) {
    showToast('Gagal terhubung ke server', 'error');
  }
}

async function togglePower() {
  if (!state.activeDevice) {
    showToast('Pilih perangkat terlebih dahulu', 'error');
    return;
  }

  const newPower = !state.power;
  state.power = newPower;
  updatePowerUI();

  if (state.connMode === 'local') {
    const base = getLocalBase();
    if (!base) {
      state.power = !newPower;
      updatePowerUI();
      showToast('Isi IP Lokal di tab Perangkat', 'error');
      return;
    }
    try {
      const res = await fetch(`${base}/api/settings`, {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ power: newPower })
      });
      if (!res.ok) throw new Error('HTTP ' + res.status);
      showToast(newPower ? 'Panel dinyalakan' : 'Panel dimatikan', 'success');
    } catch (err) {
      state.power = !newPower;
      updatePowerUI();
      showToast('Gagal ke panel lokal', 'error');
    }
    return;
  }

  try {
    const res = await apiFetch(`/api/devices/${state.activeDevice.id}/settings`, {
      method: 'PUT',
      body: JSON.stringify({ power: newPower })
    });
    showToast(newPower ? 'Panel dinyalakan' : 'Panel dimatikan', 'success');
  } catch (err) {
    state.power = !newPower;
    updatePowerUI();
    showToast('Gagal mengubah power', 'error');
  }
}

function updatePowerUI() {
  const isOn = state.power;
  if (el.btnPowerToggle) {
    el.btnPowerToggle.classList.toggle('off', !isOn);
  }
  if (el.powerStatus) {
    el.powerStatus.textContent = isOn ? 'ON' : 'OFF';
  }
  if (el.powerStatusCard) {
    el.powerStatusCard.textContent = isOn ? 'ON' : 'OFF';
  }
}

// ==================== MODAL & TOAST ====================
function showToast(message, type = 'info') {
  if (!el.toastContainer) return;
  const toast = document.createElement('div');
  toast.className = `toast ${type}`;
  const icons = {
    success: '<svg viewBox="0 0 24 24" width="14" height="14" fill="none" stroke="currentColor" stroke-width="2.5"><polyline points="20 6 9 17 4 12"/></svg>',
    error: '<svg viewBox="0 0 24 24" width="14" height="14" fill="none" stroke="currentColor" stroke-width="2.5"><line x1="18" y1="6" x2="6" y2="18"/><line x1="6" y1="6" x2="18" y2="18"/></svg>',
    info: '<svg viewBox="0 0 24 24" width="14" height="14" fill="none" stroke="currentColor" stroke-width="2.5"><circle cx="12" cy="12" r="10"/><line x1="12" y1="16" x2="12" y2="12"/><line x1="12" y1="8" x2="12.01" y2="8"/></svg>'
  };
  toast.innerHTML = `${icons[type] || icons.info} <span>${message}</span>`;
  el.toastContainer.appendChild(toast);
  setTimeout(() => {
    toast.style.transition = 'opacity 0.2s, transform 0.2s';
    toast.style.opacity = '0';
    toast.style.transform = 'translateY(-8px)';
    setTimeout(() => toast.remove(), 200);
  }, 3000);
}
