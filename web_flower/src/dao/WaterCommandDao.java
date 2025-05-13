package dao;

import model.WaterCommand;

import java.sql.*;
import java.util.ArrayList;
import java.util.List;

/**
 * WaterCommandDao - 负责对 water_command 表进行 CRUD 操作
 */
public class WaterCommandDao {

    /**
     * 新增一条浇水命令，状态初始为 "pending"
     */
    public void insertCommand(WaterCommand cmd) {
        String sql = "INSERT INTO water_command (device_id, cmd, status) VALUES (?, ?, 'pending')";
        try (Connection conn = DBUtil.getConnection();
             PreparedStatement ps = conn.prepareStatement(sql)) {

            ps.setString(1, cmd.getDeviceId());
            ps.setString(2, cmd.getCmd());
            ps.executeUpdate();

        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     * 查询指定设备最新的 pending 命令（如果树莓派轮询此命令就可以执行）
     */
    public WaterCommand queryLatestPending(String deviceId) {
        String sql = "SELECT * FROM water_command WHERE device_id=? AND status='pending' ORDER BY ts DESC LIMIT 1";
        try (Connection conn = DBUtil.getConnection();
             PreparedStatement ps = conn.prepareStatement(sql)) {

            ps.setString(1, deviceId);
            ResultSet rs = ps.executeQuery();
            if (rs.next()) {
                WaterCommand wc = new WaterCommand();
                wc.setId(rs.getInt("id"));
                wc.setDeviceId(rs.getString("device_id"));
                wc.setCmd(rs.getString("cmd"));
                wc.setStatus(rs.getString("status"));
                wc.setTimestamp(rs.getTimestamp("ts"));
                return wc;
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    /**
     * 更新命令状态，如 "done"、"processing" 等
     */
    public void updateStatus(int id, String status) {
        String sql = "UPDATE water_command SET status=? WHERE id=?";
        try (Connection conn = DBUtil.getConnection();
             PreparedStatement ps = conn.prepareStatement(sql)) {

            ps.setString(1, status);
            ps.setInt(2, id);
            ps.executeUpdate();

        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
