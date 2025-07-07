package service;

import dao.WaterCommandDao;
import model.WaterCommand;

/**
 * WaterService - 处理浇水命令相关业务
 */
public class WaterService {

    private WaterCommandDao waterCommandDao = new WaterCommandDao();

    /**
     * 创建浇水命令，p1 = 开始浇水，p0 = 停止浇水
     */
    public void createCommand(String deviceId, String cmd) {
        WaterCommand wc = new WaterCommand();
        wc.setDeviceId(deviceId);
        wc.setCmd(cmd);
        waterCommandDao.insertCommand(wc);
    }

    /**
     * 查询设备最新 pending 状态的命令
     */
    public WaterCommand getLatestPending(String deviceId) {
        return waterCommandDao.queryLatestPending(deviceId);
    }

    /**
     * 命令执行完后更新状态
     */
    public void finishCommand(int id) {
        waterCommandDao.updateStatus(id, "done");
    }
}
