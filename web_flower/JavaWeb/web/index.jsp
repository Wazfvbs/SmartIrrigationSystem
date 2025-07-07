<%@ page contentType="text/html;charset=UTF-8" language="java" %>
<jsp:include page="/header.jsp"/>

<div class="container mt-5">
  <h2>智慧花盆 - 实时数据</h2>
  <div class="row">
    <!-- Temperature Card -->
    <div class="col-md-3">
      <div class="card text-center" id="tempCard">
        <div class="card-body">
          <h5 class="card-title">环境温度</h5>
          <p id="tempValue" class="card-text display-6">--℃</p>
        </div>
      </div>
    </div>
    <!-- Humidity Card -->
    <div class="col-md-3">
      <div class="card text-center" id="humiCard">
        <div class="card-body">
          <h5 class="card-title">环境湿度</h5>
          <p id="humiValue" class="card-text display-6">--%</p>
        </div>
      </div>
    </div>
    <!-- Soil Moisture Card -->
    <div class="col-md-3">
      <div class="card text-center" id="soilCard">
        <div class="card-body">
          <h5 class="card-title">土壤湿度</h5>
          <p id="soilValue" class="card-text display-6">--</p>
        </div>
      </div>
    </div>
    <!-- Water Level Card -->
    <div class="col-md-3">
      <div class="card text-center" id="waterCard">
        <div class="card-body">
          <h5 class="card-title">水位情况</h5>
          <p id="waterValue" class="card-text display-6">--</p>
        </div>
      </div>
    </div>
  </div>

<%--  <h2 class="mt-5">24 小时数据</h2>--%>
<%--  <div class="row">--%>
<%--    <div class="col-md-12">--%>
<%--      <canvas id="tempChart" width="600" height="300"></canvas>--%>
<%--    </div>--%>
<%--    <div class="col-md-12">--%>
<%--      <canvas id="humiChart" width="600" height="300"></canvas>--%>
<%--    </div>--%>
<%--    <div class="col-md-12">--%>
<%--      <canvas id="soilChart" width="600" height="300"></canvas>--%>
<%--    </div>--%>
<%--    <div class="col-md-12">--%>
<%--      <canvas id="waterChart" width="600" height="300"></canvas>--%>
<%--    </div>--%>
<%--  </div>--%>
<%--</div>--%>

<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
<script>
  function fetchLatestData() {
    const url = "QueryData?mode=latestJson&deviceId=raspi001";
    fetch(url)
            .then(response => response.json())
            .then(data => {
              if (data.temp !== undefined) {
                document.getElementById('tempValue').innerText = data.temp + "℃";
                document.getElementById('humiValue').innerText = data.humi + "%";
                document.getElementById('soilValue').innerText = data.soil + "%";
                document.getElementById('waterValue').innerText = data.water + "cm";
              }
            })
            .catch(err => console.log("Error fetching latest data:", err));
  }

  function load24hData() {
    const url = "QueryData?mode=24hJson&deviceId=raspi001";
    fetch(url)
            .then(resp => resp.json())
            .then(data => {
              const labels = data.map(d => d.ts);
              const tempData = data.map(d => d.temp);
              const humiData = data.map(d => d.humi);
              const soilData = data.map(d => d.soil);
              const waterData = data.map(d => d.water);

              drawChart('tempChart', labels, tempData, '温度(℃)', 'red');
              drawChart('humiChart', labels, humiData, '湿度(%)', 'blue');
              drawChart('soilChart', labels, soilData, '土壤湿度', 'green');
              drawChart('waterChart', labels, waterData, '水位', 'purple');
            })
            .catch(err => console.log("Error fetching 24h data:", err));
  }

  function drawChart(canvasId, labels, dataArr, labelStr, colorStr) {
    const ctx = document.getElementById(canvasId).getContext('2d');
    new Chart(ctx, {
      type: 'line',
      data: {
        labels: labels,
        datasets: [{
          label: labelStr,
          data: dataArr,
          borderColor: colorStr,
          fill: false,
          tension: 0.1
        }]
      },
      options: {
        responsive: true,
        scales: {
          x: {
            ticks: { maxRotation: 90, minRotation: 30 }
          }
        }
      }
    });
  }

  setInterval(fetchLatestData, 5000);
  document.addEventListener("DOMContentLoaded", () => {
    fetchLatestData();
    load24hData();
  });
</script>

<jsp:include page="/footer.jsp"/>
