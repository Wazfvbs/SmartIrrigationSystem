<%@ page contentType="text/html;charset=UTF-8" language="java" %>
<jsp:include page="/header.jsp"/>

<h2>24 小时数据</h2>

<div class="row">
    <div class="col-md-12">
        <canvas id="tempChart" width="600" height="300"></canvas>
    </div>
    <div class="col-md-12">
        <canvas id="humiChart" width="600" height="300"></canvas>
    </div>
    <div class="col-md-12">
        <canvas id="soilChart" width="600" height="300"></canvas>
    </div>
    <div class="col-md-12">
        <canvas id="waterChart" width="600" height="300"></canvas>
    </div>
</div>

<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
<script>
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

    document.addEventListener("DOMContentLoaded", load24hData);
</script>

<jsp:include page="/footer.jsp"/>
