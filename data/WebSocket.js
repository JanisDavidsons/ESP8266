var rgbOn = false;
let graphLoggingOn = false;

let rgbData = {
  rgb: {
    red: 0,
    green: 0,
    blue: 0,
  },
};

var connection = new WebSocket("ws://" + location.hostname + ":81/", [
  "arduino",
]);

connection.onopen = function () {
  connection.send("Connect " + new Date());
  getTemperature();
  getRgbData();
  getInitData();
};
connection.onerror = function (error) {
  console.log("WebSocket Error ", error);
};
connection.onmessage = (event) => {
  processReceivedCommand(event);
};

connection.onclose = function () {
  console.log("WebSocket connection closed");
};

function sendRGB() {
  rgbData.rgb.red = document.getElementById("r").value;
  rgbData.rgb.green = document.getElementById("g").value;
  rgbData.rgb.blue = document.getElementById("b").value;
  connection.send(JSON.stringify(rgbData));
}

function toggleRGB() {
  rgbOn = !rgbOn;
  if (rgbOn) {
    connection.send(JSON.stringify({ rgbOff: {} }));
    document.getElementById("toggleRGB").style.backgroundColor = "#00878F";
    document.getElementById("r").className = "disabled";
    document.getElementById("g").className = "disabled";
    document.getElementById("b").className = "disabled";
    document.getElementById("r").disabled = true;
    document.getElementById("g").disabled = true;
    document.getElementById("b").disabled = true;
  } else {
    connection.send(JSON.stringify({ rgbOn: {} }));
    document.getElementById("toggleRGB").style.backgroundColor = "#999";
    document.getElementById("r").className = "enabled";
    document.getElementById("g").className = "enabled";
    document.getElementById("b").className = "enabled";
    document.getElementById("r").disabled = false;
    document.getElementById("g").disabled = false;
    document.getElementById("b").disabled = false;

    getRgbData();
  }
}

function processReceivedCommand(event) {
  let data = JSON.parse(event.data);

  switch (true) {
    case data.hasOwnProperty("temp"):
      console.log(data.temp.value);
      document.getElementById("temperature-C").innerHTML = data.temp.value;
      break;
    case data.hasOwnProperty("rgbData"):
      console.log(data.rgbData);
      document.getElementById("r").value = data.rgbData.red;
      document.getElementById("g").value = data.rgbData.green;
      document.getElementById("b").value = data.rgbData.blue;
      break;
    case data.hasOwnProperty("initData"):
      console.log(data.initData);

      document.getElementById("r").value = data.initData.rgbData.red;
      document.getElementById("g").value = data.initData.rgbData.green;
      document.getElementById("b").value = data.initData.rgbData.blue;

      graphLoggingOn = data.initData.graphLogData.logging;
      document.getElementById("logGraph").innerHTML = graphLoggingOn
        ? "Stop Logging"
        : "Start Logging";

      document.getElementById("temperature-C").innerHTML =
        data.initData.tempData.currentTemp;

      document.getElementById("logIntervalValue").innerHTML =
        data.initData.logInterval.value;

      document.getElementById("logIntervalSlider").value =
        data.initData.logInterval.value;
  }
}

function getTemperature() {
  connection.send(JSON.stringify({ temp: {} }));
}

function deleteCsv() {
  connection.send(JSON.stringify({ clearLogFile: {} }));
}

const getRgbData = () => {
  connection.send(JSON.stringify({ getRgbData: {} }));
};

const logGraph = () => {
  graphLoggingOn = !graphLoggingOn;
  if (graphLoggingOn) {
    document.getElementById("logGraph").innerHTML = "Stop Logging";
  } else {
    document.getElementById("logGraph").innerHTML = "Start Logging";
  }
  connection.send(JSON.stringify({ logGraph: { value: graphLoggingOn } }));
};

const getInitData = () => {
  connection.send(JSON.stringify({ getInitData: {} }));
};

const setLogInterval = () => {
  let interval = document.getElementById("logIntervalSlider").value;
  document.getElementById("logIntervalValue").innerHTML = interval;
  console.log(interval);
  connection.send(JSON.stringify({ logInterval: interval }));
};
