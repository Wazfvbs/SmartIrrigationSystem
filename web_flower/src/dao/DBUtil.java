package dao;

import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.SQLException;

public class DBUtil {
    // 数据库URL、用户名和密码
    private static final String URL = "jdbc:mysql://localhost:3306/smart_flower_db?useSSL=false&serverTimezone=UTC";
    private static final String USER = "root"; // 替换为你的数据库用户名
    private static final String PASSWORD = "a523351447"; // 替换为你的数据库密码

    // 静态方法获取数据库连接
    public static Connection getConnection() {
        Connection connection = null;
        try {
            // 加载MySQL驱动
            Class.forName("com.mysql.cj.jdbc.Driver");
            // 获取连接
            connection = DriverManager.getConnection(URL, USER, PASSWORD);
        } catch (ClassNotFoundException e) {
            System.out.println("数据库驱动加载失败！");
            e.printStackTrace();
        } catch (SQLException e) {
            System.out.println("数据库连接失败！");
            e.printStackTrace();
        }
        return connection;
    }

    // 关闭连接的方法
    public static void closeConnection(Connection connection) {
        try {
            if (connection != null) {
                connection.close();
            }
        } catch (SQLException e) {
            System.out.println("关闭数据库连接失败！");
            e.printStackTrace();
        }
    }
}
