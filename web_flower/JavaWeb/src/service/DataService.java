package service;


import dao.SensorDataDao;
import model.SensorData;

import java.util.List;

/**
 * DataService - 处理传感器数据相关的业务逻辑
 */
public class DataService {

    private SensorDataDao sensorDataDao = new SensorDataDao();

    /**
     * 保存一条新的传感器数据
     */
    public void saveData(SensorData data) {
        sensorDataDao.insertSensorData(data);
    }

    /**
     * 获取指定设备最新一条数据
     */
    public SensorData getLatestData(String deviceId) {
        return sensorDataDao.queryLatest(deviceId);
    }

    /**
     * 获取指定设备的最近 24 小时数据
     */
    public List<SensorData> getLast24HoursData(String deviceId) {
        return sensorDataDao.queryLast24Hours(deviceId);
    }
}
