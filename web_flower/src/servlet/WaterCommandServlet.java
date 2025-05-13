package servlet;

import service.WaterService;

import javax.servlet.ServletException;
import javax.servlet.http.*;
import java.io.IOException;

/**
 * WaterCommandServlet - 处理前端发起的浇水命令
 * 路径: /WaterCommand
 * 参数: deviceId=xxx, cmd=p1(开始)或p0(停止)
 */
public class WaterCommandServlet extends HttpServlet {

    private WaterService waterService = new WaterService();

    @Override
    protected void doPost(HttpServletRequest req, HttpServletResponse resp)
            throws ServletException, IOException {
        req.setCharacterEncoding("UTF-8");

        String deviceId = req.getParameter("deviceId");
        String cmd = req.getParameter("cmd"); // "p1" or "p0"

        // 创建浇水命令
        waterService.createCommand(deviceId, cmd);

        // 操作完成后, 跳回首页或刷新当前页
        resp.sendRedirect("index.jsp");
    }
}
