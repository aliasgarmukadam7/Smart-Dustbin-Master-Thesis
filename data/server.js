const express = require("express");
const path = require("path");

const app = express();
const PORT = 8000;

app.use(express.json());
app.use(express.static(__dirname));

let state = {
  distance_cm: 24.5,
  fill_pct: 38,
  ir: 0,
  lid_open: 0,
  water_value: 820,
  water_detected: 0,
  buzzer_on: 1
};

// tracks whether lid was opened manually
let manualLidOpen = false;
let manualAlarm = true;

function clamp(v, min, max) {
  return Math.max(min, Math.min(max, v));
}

function updateState() {
  state.fill_pct = clamp(
    state.fill_pct + Math.round((Math.random() * 8) - 4),
    0,
    100
  );

  state.distance_cm = Math.max(5, 40 - state.fill_pct * 0.3);

  // Only allow automatic IR activity when lid is NOT manually opened
  if (!manualLidOpen && Math.random() > 0.9) {
    state.ir = 1;
    state.lid_open = 1;

    setTimeout(() => {
      // clear IR only if still not in manual mode
      if (!manualLidOpen) {
        state.ir = 0;
      }
    }, 1200);

    setTimeout(() => {
      // auto-close only if still not in manual mode
      if (!manualLidOpen) {
        state.lid_open = 0;
      }
    }, 2800);
  } else if (manualLidOpen) {
    // force IR inactive during manual lid open
    state.ir = 0;
  }

  if (Math.random() > 0.93) {
    state.water_detected = 1;
    state.water_value = 1800 + Math.floor(Math.random() * 300);
  } else {
    state.water_detected = 0;
    state.water_value = 700 + Math.floor(Math.random() * 300);
  }

  // state.buzzer_on = state.fill_pct >= 90 ? (Math.random() > 0.5 ? 1 : 0) : 0;
  if (!manualAlarm) {
  state.buzzer_on = state.fill_pct >= 90 ? (Math.random() > 0.5 ? 1 : 0) : 0;
}
}

setInterval(updateState, 1000);

app.get("/data", (req, res) => {
  res.set("Cache-Control", "no-store");
  res.json(state);
});

app.post("/lid/open", (req, res) => {
  manualLidOpen = true;
  state.lid_open = 1;
  state.ir = 0; // no IR activity when manually opened
  res.json({ ok: 1, cmd: "open", manual: 1 });
});

app.post("/lid/close", (req, res) => {
  manualLidOpen = false;
  state.lid_open = 0;
  state.ir = 0;
  res.json({ ok: 1, cmd: "close", manual: 0 });
});
app.post("/alarm/on", (req, res) => {
  manualAlarm = true;
  state.buzzer_on = 1;
  res.json({ ok: 1, alarm: "on", manual: 1 });
});

app.post("/alarm/off", (req, res) => {
  manualAlarm = false;
  state.buzzer_on = 0;
  res.json({ ok: 1, alarm: "off", manual: 0 });
});

app.get("/", (req, res) => {
  res.sendFile(path.join(__dirname, "index.html"));
});

app.listen(PORT, () => {
  console.log(`Mock server running at http://localhost:${PORT}`);
});