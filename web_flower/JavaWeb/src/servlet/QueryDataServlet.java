package servlet;

import model.SensorData;
import service.DataService;

import javax.servlet.ServletException;
import javax.servlet.http.*;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.List;

/**
 * QueryDataServlet - 处理实时数据请求和24小时数据请求
 */
public class QueryDataServlet extends HttpServlet {

    private DataService dataService = new DataService();

    @Override
    protected void doGet(HttpServletRequest req, HttpServletResponse resp)
            throws ServletException, IOException {
        req.setCharacterEncoding("UTF-8");

        String mode = req.getParameter("mode");
        String deviceId = req.getParameter("deviceId");

        if ("latestJson".equals(mode)) {
            // 查询最新数据
            SensorData latestData = dataService.getLatestData(deviceId);
            if (latestData != null) {
                resp.setContentType("application/json;charset=UTF-8");
                PrintWriter out = resp.getWriter();
                out.print("{");
                out.print("\"temp\": " + latestData.getTemperature() + ",");
                out.print("\"humi\": " + latestData.getHumidity() + ",");
                out.print("\"soil\": " + latestData.getSoilMoisture() + ",");
                out.print("\"water\": " + latestData.getWaterLevel() + ",");
                out.print("\"timestamp\": \"" + latestData.getTimestamp() + "\"");
                out.print("}");
            }
        } else if ("24hJson".equals(mode)) {
            // 查询24小时数据
            List<SensorData> dataList = dataService.getLast24HoursData(deviceId);
            resp.setContentType("application/json;charset=UTF-8");
            PrintWriter out = resp.getWriter();
            out.print("[");
            for (int i = 0; i < dataList.size(); i++) {
                SensorData sd = dataList.get(i);
                String json = String.format(
                        "{\"ts\":\"%s\", \"temp\":%.2f, \"humi\":%.2f, \"soil\":%.2f, \"water\":%.2f}",
                        sd.getTimestamp(), sd.getTemperature(), sd.getHumidity(),
                        sd.getSoilMoisture(), sd.getWaterLevel()
                );
                out.print(json);
                if (i < dataList.size() - 1) {
                    out.print(",");
                }
            }
            out.print("]");
        }
    }
}
