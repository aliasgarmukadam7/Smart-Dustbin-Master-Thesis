window.addEventListener("DOMContentLoaded", function () {
  var $ = function (id) { return document.getElementById(id); };

  var UI = {
    conn: $("conn"),
    dbg: $("dbg"),
    dot: $("dot"),
    dist: $("dist"),
    fillBig: $("fill"),
    ir: $("ir"),
    lid: $("lid"),
    water: $("water"),
    alarm: $("alarm"),
    prog: $("prog"),
    statusPill: $("statusPill"),
    binState: $("binState"),
    c_ir: $("c_ir"),
    c_lid: $("c_lid"),
    c_fill: $("c_fill"),
    c_water: $("c_water"),
    c_alarm: $("c_alarm"),
    led_ir: $("led_ir"),
    led_lid: $("led_lid"),
    led_fill: $("led_fill"),
    led_water: $("led_water"),
    led_alarm: $("led_alarm"),
    log: $("log"),
    chart: $("chart"),
    ip: $("ip"),
    btnOpen: $("btnOpen"),
    btnClose: $("btnClose"),
    alarmOn: $("alarmon"),
    alarmOff: $("alarmoff"),
    handIndicator: $("handIndicator"),
    lidAnim: $("lidAnim"),
    binFillAnim: $("binFillAnim"),
    garbageAnim: $("garbageAnim"),
    sceneStatus: $("sceneStatus")
  };

  function logMsg(t) {if (UI.dbg) UI.dbg.textContent = t;}

  function clamp(v, a, b) {return Math.max(a, Math.min(b, v));}

  function lerp(a, b, t) {return a + (b - a) * t;}

  function setDot(ok) {
    if (!UI.dot) return;
    UI.dot.style.background = ok ? "#2dff84" : "#ffe45e";
    UI.dot.style.boxShadow = ok  ? "0 0 18px rgba(45,255,132,.35)" : "0 0 18px rgba(255,228,94,.35)";
  }

  var UI4 = {
    targetFill: 0,
    dispFill: 0,
    targetDist: 0,
    dispDist: 0,
    targetIr: 0,
    targetLid: 0,
    targetWater: 0,
    targetBuzzer: 0,
    targetAlert: 0,
    targetColor: "#2dff84",
    lastState: "Empty",
    lastTs: 0,
    series: [],
    seriesMax: 120,
    animStarted: false,
    lastSnapshot: "",
    sceneAnimRunning: false,
    sceneResetTimer: null
  };

  var grad = document.getElementById("gradFill");
  var gradAngle = 0;

  function animateGrad() {
    if (grad) {
      gradAngle = (gradAngle + 0.8) % 360;
      grad.setAttribute("gradientTransform", "rotate(" + gradAngle + " 0.5 0.5)");
    }
    requestAnimationFrame(animateGrad);
  }
  requestAnimationFrame(animateGrad);

  function fillToColor(fill) {
    if (fill >= 90) return { c: "#ff1e3c", s: "FULL" };
    if (fill >= 75) return { c: "#ff6b6b", s: "High" };
    if (fill >= 50) return { c: "#ffb020", s: "Medium" };
    if (fill >= 25) return { c: "#ffe45e", s: "Low" };
    return { c: "#2dff84", s: "Empty" };
  }

  function setRingColor(color) {if (UI.prog) UI.prog.style.stroke = color;}

  function pushActivity(message, tag) {
    if (!UI.log) return;

    var row = document.createElement("div");
    row.className = "row";

    var left ="<div class='left'>" + "<div class='m'>" + message + "</div>" + "<div class='t'>" + new Date().toLocaleTimeString() + "</div>" + "</div>";
    var right = "<div class='tag'>" + tag + "</div>";

    row.innerHTML = left + right;
    UI.log.prepend(row);

    while (UI.log.children.length > 12) {
      UI.log.removeChild(UI.log.lastChild);
    }
  }

  function updateFillVisual(fillPct) {
    if (!UI.binFillAnim) return;
    var h = clamp(fillPct, 8, 95);
    UI.binFillAnim.style.height = h + "%";

    if (fillPct >= 85) {
      UI.binFillAnim.style.background = "linear-gradient(180deg,#ef4444 0%,#b91c1c 100%)";
    } else if (fillPct >= 55) {
      UI.binFillAnim.style.background = "linear-gradient(180deg,#f59e0b 0%,#d97706 100%)";
    } else {
      UI.binFillAnim.style.background = "linear-gradient(180deg,#22c55e 0%,#15803d 100%)";
    }
  }


  function setSceneStatus(txt) {
    if (UI.sceneStatus) UI.sceneStatus.textContent = txt;
  }

  function updateLidVisual(open) {
    if (!UI.lidAnim) return;

    UI.lidAnim.classList.toggle("open", open);

    if (!UI4.sceneAnimRunning) {
      setSceneStatus(open ? "LID OPEN" : "Waiting to detect hand");
    }
  }

  function runDropAnimation() {
    if (UI4.sceneAnimRunning) return;
    UI4.sceneAnimRunning = true;

    if (UI.handIndicator) UI.handIndicator.classList.add("active");
    setSceneStatus("HAND DETECTED");

    setTimeout(() => {
      if (UI.lidAnim) UI.lidAnim.classList.add("open");
      setSceneStatus("LID OPENING");
    }, 500);

    setTimeout(() => {
      if (UI.garbageAnim) {
        UI.garbageAnim.classList.remove("drop");
        void UI.garbageAnim.offsetWidth;
        UI.garbageAnim.classList.add("drop");
      }
      setSceneStatus("WASTE DROPPING");
    }, 1200);

    setTimeout(() => {
      setSceneStatus("DISPOSAL COMPLETE");
    }, 2200);

    if (UI4.sceneResetTimer) clearTimeout(UI4.sceneResetTimer);
      UI4.sceneResetTimer = setTimeout(() => {
      if (UI.handIndicator) UI.handIndicator.classList.remove("active");
      UI4.sceneAnimRunning = false;

      setSceneStatus(UI4.targetLid ? "LID OPEN" : "Waiting to detect hand");
    }, 3200);
  }

  function feedData(d) {
    var prevIr = UI4.targetIr;

    var fill = Number(d.fill_pct);
    var dist = Number(d.distance_cm);
    var ir = Number(d.ir);
    var lid = Number(d.lid_open);
    var water = Number(d.water_detected);
    var buzzer = Number(d.buzzer_on);
    var alert = Number(d.alert_active);

    if (!isFinite(fill)) fill = 0;
    if (!isFinite(dist)) dist = 0;

    UI4.targetFill = clamp(fill, 0, 100);
    UI4.targetDist = dist;
    UI4.targetIr = ir ? 1 : 0;
    UI4.targetLid = lid ? 1 : 0;
    UI4.targetWater = water ? 1 : 0;
    UI4.targetBuzzer = buzzer ? 1 : 0;
    UI4.targetAlert = alert ? 1 : 0;

    var cs = fillToColor(UI4.targetFill);
    UI4.targetColor = cs.c;
    UI4.lastState = cs.s;

    UI4.series.push(UI4.targetFill);
    if (UI4.series.length > UI4.seriesMax) UI4.series.shift();

    updateFillVisual(UI4.targetFill);
    updateLidVisual(UI4.targetLid);
        // Sync Lid buttons with actual state
    if (UI.btnOpen && UI.btnClose) {
      if (UI4.targetLid === 1) {
        UI.btnOpen.disabled = true;
        UI.btnClose.disabled = false;
      } else {
        UI.btnOpen.disabled = false;
        UI.btnClose.disabled = true;
      }
    }

    // Sync Alarm buttons
    if (UI.alarmOn && UI.alarmOff) {
      if (UI4.targetBuzzer === 1) {
        UI.alarmOn.disabled = true;
        UI.alarmOff.disabled = false;
      } else {
        UI.alarmOn.disabled = false;
        UI.alarmOff.disabled = true;
      }
    }

    if (UI4.targetIr === 1 && prevIr === 0) {
      runDropAnimation();
    }

    var snap = [
      Math.round(UI4.targetFill),
      UI4.targetIr,
      UI4.targetLid,
      UI4.targetWater,
      UI4.targetBuzzer,
      UI4.lastState
    ].join("|");

    if (snap !== UI4.lastSnapshot) {
      var msg = "Fill: " + Math.round(UI4.targetFill) + "% • Dist: " + UI4.targetDist.toFixed(2) + " cm";
      if (UI4.targetIr) msg += " • Hand detected";
      if (UI4.targetWater) msg += " • Wet waste likely";
      if (UI4.targetBuzzer) msg += " • Alarm beeping";

      pushActivity(msg, UI4.targetLid ? "OPEN" : "CLOSED");
      UI4.lastSnapshot = snap;
    }

    if (!UI4.animStarted) {
      UI4.animStarted = true;
      requestAnimationFrame(tickAnim);
    }


  }


  function drawChart() {
    if (!UI.chart) return;

    var canvas = UI.chart;
    var ctx = canvas.getContext("2d");

    var rect = canvas.getBoundingClientRect();
    var dpr = window.devicePixelRatio || 1;

    var displayWidth = Math.max(1, Math.round(rect.width));
    var displayHeight = Math.max(1, Math.round(rect.height));

    if (canvas.width !== displayWidth * dpr || canvas.height !== displayHeight * dpr) {
      canvas.width = displayWidth * dpr;
      canvas.height = displayHeight * dpr;
    }

    ctx.setTransform(1, 0, 0, 1, 0, 0);
    ctx.scale(dpr, dpr);

    var w = displayWidth;
    var h = displayHeight;

    var padLeft = 58;
    var padRight = 16;
    var padTop = 14;
    var padBottom = 34;

    var plotW = w - padLeft - padRight;
    var plotH = h - padTop - padBottom;

    ctx.clearRect(0, 0, w, h);

    if (plotW <= 0 || plotH <= 0) return;

    function yOf(v) {
      return padTop + plotH - (clamp(v, 0, 100) * plotH / 100);
    }

    function xOf(i) {
      var denom = Math.max(1, UI4.seriesMax - 1);
      return padLeft + (i * plotW / denom);
    }

    ctx.strokeStyle = "rgba(255,255,255,0.10)";
    ctx.lineWidth = 1;

    ctx.beginPath();
    ctx.moveTo(padLeft, padTop);
    ctx.lineTo(padLeft, padTop + plotH);
    ctx.stroke();

    ctx.beginPath();
    ctx.moveTo(padLeft, padTop + plotH);
    ctx.lineTo(padLeft + plotW, padTop + plotH);
    ctx.stroke();

    // GRID + Y AXIS LABELS
    ctx.lineWidth = 1;
    ctx.strokeStyle = "rgba(255,255,255,0.08)";
    ctx.fillStyle = "rgba(255,255,255,0.88)";
    ctx.font = "14px system-ui, sans-serif";
    ctx.textAlign = "right";
    ctx.textBaseline = "middle";

    for (var i = 0; i <= 4; i++) {
      var value = 100 - (i * 25);
      var y = yOf(value);

      ctx.beginPath();
      ctx.moveTo(padLeft, y);
      ctx.lineTo(w - padRight, y);
      ctx.stroke();

      ctx.fillText(value + "%", padLeft - 12, y);
    }

    // X AXIS LABELS
    ctx.textAlign = "center";
    ctx.textBaseline = "top";
    ctx.fillStyle = "rgba(255,255,255,0.72)";
    ctx.font = "13px system-ui, sans-serif";

    var xLabels = [
      { pos: 0.00, label: "-60s" },
      { pos: 0.25, label: "-45s" },
      { pos: 0.50, label: "-30s" },
      { pos: 0.75, label: "-15s" },
      { pos: 1.00, label: "now"  }
    ];

    for (var t = 0; t < xLabels.length; t++) {
      var x = padLeft + (plotW * xLabels[t].pos);

      ctx.strokeStyle = "rgba(255,255,255,0.04)";
      ctx.beginPath();
      ctx.moveTo(x, padTop);
      ctx.lineTo(x, padTop + plotH);
      ctx.stroke();

      ctx.fillText(xLabels[t].label, x, padTop + plotH + 8);
    }

    var n = UI4.series.length;
    if (n < 2) return;

    ctx.save();
    ctx.lineJoin = "round";
    ctx.lineCap = "round";

    // Glow line
    ctx.strokeStyle = "rgba(255,176,32,0.18)";
    ctx.lineWidth = 10;
    ctx.beginPath();
    for (var k = 0; k < n; k++) {
      var x1 = xOf(k);
      var y1 = yOf(UI4.series[k]);
      if (k === 0) ctx.moveTo(x1, y1);
      else ctx.lineTo(x1, y1);
    }
    ctx.stroke();

    // Main line
    ctx.strokeStyle = "rgba(255,228,94,0.96)";
    ctx.lineWidth = 3;
    ctx.beginPath();
    for (var j = 0; j < n; j++) {
      var x2 = xOf(j);
      var y2 = yOf(UI4.series[j]);
      if (j === 0) ctx.moveTo(x2, y2);
      else ctx.lineTo(x2, y2);
    }
    ctx.stroke();

    // Latest point marker
    var lx = xOf(n - 1);
    var ly = yOf(UI4.series[n - 1]);

    ctx.strokeStyle = "rgba(106,169,255,0.60)";
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(lx, padTop);
    ctx.lineTo(lx, padTop + plotH);
    ctx.stroke();

    ctx.fillStyle = "rgba(255,255,255,0.95)";
    ctx.beginPath();
    ctx.arc(lx, ly, 5, 0, Math.PI * 2);
    ctx.fill();

    // Current value label near latest point
    var label = Math.round(UI4.series[n - 1]) + "%";
    ctx.fillStyle = "rgba(255,255,255,0.95)";
    ctx.font = "bold 14px system-ui, sans-serif";
    ctx.textBaseline = "bottom";

    var textW = ctx.measureText(label).width;
    var gap = 8;
    var labelX = lx + gap;
    var labelY = ly - 8;

    // If label would overflow right edge, place it to the left
    if (labelX + textW > w - padRight) {
      ctx.textAlign = "right";
      labelX = lx - gap;
    } else {
      ctx.textAlign = "left";
    }

    // If too close to top, place it below the point
    if (labelY < padTop + 14) {
      ctx.textBaseline = "top";
      labelY = ly + 8;
    }

    ctx.fillText(label, labelX, labelY);

    ctx.restore();
  }

  function tickAnim(ts) {
    if (!UI4.lastTs) UI4.lastTs = ts;

    var dt = (ts - UI4.lastTs) / 1000.0;
    UI4.lastTs = ts;
    dt = clamp(dt, 0.0, 0.05);

    var k = 1 - Math.pow(0.001, dt);

    UI4.dispFill = lerp(UI4.dispFill, UI4.targetFill, k);
    UI4.dispDist = lerp(UI4.dispDist, UI4.targetDist, k);

    if (UI.fillBig) UI.fillBig.textContent = Math.round(UI4.dispFill) + "%";
    if (UI.dist) UI.dist.textContent = UI4.dispDist.toFixed(2) + " cm";
    if (UI.ir) UI.ir.textContent = UI4.targetIr ? "Detected" : "None";
    if (UI.lid) UI.lid.textContent = UI4.targetLid ? "Open" : "Closed";
    if (UI.water) UI.water.textContent = UI4.targetWater ? "Wet waste" : "Dry / Normal";
    if (UI.alarm) UI.alarm.textContent = UI4.targetAlert ? "ON (FULL ALERT)" : (UI4.targetBuzzer ? "MANUAL ON" : "OFF");
    if (UI.statusPill) UI.statusPill.textContent = UI4.lastState;
    if (UI.binState) UI.binState.textContent = UI4.lastState;

    if (UI.prog) {
      var circumference = 490;
      var offset = circumference - (UI4.dispFill / 100) * circumference;
      UI.prog.style.strokeDashoffset = offset;
    }

    setRingColor(UI4.targetColor);

    if (UI.led_ir) UI.led_ir.style.background = UI4.targetIr ? "#ffb020" : "#222";
    if (UI.led_lid) UI.led_lid.style.background = UI4.targetLid ? "#2dff84" : "#ff1e3c";
    if (UI.led_fill) UI.led_fill.style.background = UI4.targetColor;
    if (UI.led_water) UI.led_water.style.background = UI4.targetWater ? "#4da6ff" : "#222";
    if (UI.led_alarm) UI.led_alarm.style.background = UI4.targetAlert ? "#ff1e3c" : (UI4.targetBuzzer ? "#ffb020" : "#222"); //UI4.targetBuzzer ? "#ff1e3c" : "#222";

    if (UI.c_ir) UI.c_ir.textContent = "IR: " + (UI4.targetIr ? "ON" : "OFF");
    if (UI.c_lid) UI.c_lid.textContent = "Lid: " + (UI4.targetLid ? "OPEN" : "CLOSED");
    if (UI.c_fill) UI.c_fill.textContent = "Fill: " + Math.round(UI4.dispFill) + "%";
    if (UI.c_water) UI.c_water.textContent = "Wet: " + (UI4.targetWater ? "YES" : "NO");
    if (UI.c_alarm) UI.c_alarm.textContent = "Alarm: " + (UI4.targetAlert ? "FULL" : (UI4.targetBuzzer ? "MANUAL" : "OFF")); //"Alarm: " + (UI4.targetBuzzer ? "ON" : "OFF");

    drawChart();
    requestAnimationFrame(tickAnim);
  }

  async function sendLid(url, label) {
    try {
      var r = await fetch(url, { method: "POST" });
      if (!r.ok) throw new Error("HTTP " + r.status);
      pushActivity(label, "CTRL");
      poll();
    } catch (e) {
      logMsg("ERR: " + e);
    }
  }

  if (UI.btnOpen && UI.btnClose) {
    UI.btnOpen.addEventListener("click", async function () {

      await sendLid("/lid/open", "Manual open lid");

      // toggle buttons
      UI.btnOpen.disabled = true;
      UI.btnClose.disabled = false;
    });

    UI.btnClose.addEventListener("click", async function () {
      await sendLid("/lid/close", "Manual close lid");

      // toggle buttons
      UI.btnClose.disabled = true;
      UI.btnOpen.disabled = false;
    });
  }


  async function sendAlarm(url, label) {
    try {
      var r = await fetch(url, { method: "POST" });
      if (!r.ok) throw new Error("HTTP " + r.status);

      pushActivity(label, "ALARM");
      poll();
    } catch (e) {
      logMsg("ERR: " + e);
    }
  }

  if (UI.alarmOn && UI.alarmOff) {
    UI.alarmOn.addEventListener("click", async function () {
      await sendAlarm("/alarm/on", "Alarm turned ON");
      
      // toggle buttons
      UI.alarmOn.disabled = true;
      UI.alarmOff.disabled = false;
    });

    UI.alarmOff.addEventListener("click", async function () {
      await sendAlarm("/alarm/off", "Alarm turned OFF");
     
      // toggle buttons
      UI.alarmOff.disabled = true;
      UI.alarmOn.disabled = false;
    });
  }

  async function poll() {
    try {
      var url = window.location.origin + "/data?ts=" + Date.now();
      var r = await fetch(url, { cache: "no-store" });
      if (!r.ok) throw new Error("HTTP " + r.status);

      var d = await r.json();
      logMsg("OK: " + JSON.stringify(d));
      feedData(d);

      if (UI.conn) UI.conn.textContent = "Live ✅";
      if (UI.ip) UI.ip.textContent = window.location.host;
      setDot(true);
    } catch (e) {
      logMsg("ERR: " + e);
      if (UI.conn) UI.conn.textContent = "Offline ❌";
      setDot(false);
    }
  }

  setInterval(poll, 500);
  poll();
});

