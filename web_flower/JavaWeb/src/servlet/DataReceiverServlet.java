package servlet;

import model.SensorData;
import service.DataService;

import javax.servlet.ServletException;
import javax.servlet.http.*;
import java.io.IOException;

/**
 * DataReceiverServlet - 树莓派通过 HTTP POST 向此 Servlet 上传传感器数据
 * 路径示例: /DataReceiver
 */
public class DataReceiverServlet extends HttpServlet {

    private DataService dataService = new DataService();

    @Override
    protected void doPost(HttpServletRequest req, HttpServletResponse resp)
            throws ServletException, IOException {

        // 解决中文请求参数乱码
        req.setCharacterEncoding("UTF-8");

        // 假设树莓派以 Form-Data 方式上传
        // 如果以 JSON 格式上传，需要使用流读取 + JSON 解析
        String deviceId = req.getParameter("deviceId");
        String tempStr   = req.getParameter("temp");
        String humiStr   = req.getParameter("humi");
        String soilStr   = req.getParameter("soil");
        String waterStr  = req.getParameter("water");

        // 做一些容错处理（此处省略）
        if (deviceId == null) {
            deviceId = "unknown";
        }

        // 将参数转换为数值
        double tempVal   = tempStr  != null ? Double.parseDouble(tempStr)  : 0.0;
        double humiVal   = humiStr  != null ? Double.parseDouble(humiStr)  : 0.0;
        double soilVal   = soilStr  != null ? Double.parseDouble(soilStr)  : 0.0;
        double waterVal  = waterStr != null ? Double.parseDouble(waterStr) : 0.0;

        // 封装成对象
        SensorData sd = new SensorData();
        sd.setDeviceId(deviceId);
        sd.setTemperature(tempVal);
        sd.setHumidity(humiVal);
        sd.setSoilMoisture(soilVal);
        sd.setWaterLevel(waterVal);

        // 存储到数据库
        dataService.saveData(sd);

        // 返回简单的响应
        resp.setContentType("text/plain;charset=UTF-8");
        resp.getWriter().println("OK");
    }
}
