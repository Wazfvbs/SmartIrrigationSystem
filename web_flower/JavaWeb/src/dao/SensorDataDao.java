package dao;


import model.SensorData;

import java.sql.*;
import java.util.ArrayList;
import java.util.List;

/**
 * SensorDataDao - 负责对 sensor_data 表进行 CRUD 操作
 */
public class SensorDataDao {

    /**
     * 插入一条新的传感器数据记录
     */
    public void insertSensorData(SensorData data) {
        String sql = "INSERT INTO sensor_data (device_id, temperature, humidity, soil_moisture, water_level) "
                + "VALUES (?, ?, ?, ?, ?)";
        try (Connection conn = DBUtil.getConnection();
             PreparedStatement ps = conn.prepareStatement(sql)) {

            ps.setString(1, data.getDeviceId());
            ps.setDouble(2, data.getTemperature());
            ps.setDouble(3, data.getHumidity());
            ps.setDouble(4, data.getSoilMoisture());
            ps.setDouble(5, data.getWaterLevel());
            ps.executeUpdate();

        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     * 查询指定设备 24 小时内的数据，按时间升序
     */
    public List<SensorData> queryLast24Hours(String deviceId) {
        List<SensorData> list = new ArrayList<>();
        String sql = "SELECT * FROM sensor_data "
                + "WHERE device_id=? AND ts >= (NOW() - INTERVAL 24 HOUR) "
                + "ORDER BY ts ASC";
        try (Connection conn = DBUtil.getConnection();
             PreparedStatement ps = conn.prepareStatement(sql)) {

            ps.setString(1, deviceId);
            ResultSet rs = ps.executeQuery();
            while (rs.next()) {
                SensorData d = new SensorData();
                d.setId(rs.getInt("id"));
                d.setDeviceId(rs.getString("device_id"));
                d.setTemperature(rs.getDouble("temperature"));
                d.setHumidity(rs.getDouble("humidity"));
                d.setSoilMoisture(rs.getDouble("soil_moisture"));
                d.setWaterLevel(rs.getDouble("water_level"));
                d.setTimestamp(rs.getTimestamp("ts"));
                list.add(d);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return list;
    }

    /**
     * 查询指定设备的最新一条数据
     */
    public SensorData queryLatest(String deviceId) {
        String sql = "SELECT * FROM sensor_data WHERE device_id=? ORDER BY ts DESC LIMIT 1";
        try (Connection conn = DBUtil.getConnection();
             PreparedStatement ps = conn.prepareStatement(sql)) {

            ps.setString(1, deviceId);
            ResultSet rs = ps.executeQuery();
            if (rs.next()) {
                SensorData d = new SensorData();
                d.setId(rs.getInt("id"));
                d.setDeviceId(rs.getString("device_id"));
                d.setTemperature(rs.getDouble("temperature"));
                d.setHumidity(rs.getDouble("humidity"));
                d.setSoilMoisture(rs.getDouble("soil_moisture"));
                d.setWaterLevel(rs.getDouble("water_level"));
                d.setTimestamp(rs.getTimestamp("ts"));
                return d;
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }
}
