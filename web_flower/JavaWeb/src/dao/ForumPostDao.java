package dao;

import model.ForumPost;

import java.sql.*;
import java.util.ArrayList;
import java.util.List;

/**
 * ForumPostDao - 论坛帖子表的 CRUD
 */
public class ForumPostDao {

    /**
     * 新增一个帖子
     */
    public void insertPost(ForumPost post) {
        String sql = "INSERT INTO forum_post (title, content, author) VALUES (?, ?, ?)";
        try (Connection conn = DBUtil.getConnection();
             PreparedStatement ps = conn.prepareStatement(sql)) {

            ps.setString(1, post.getTitle());
            ps.setString(2, post.getContent());
            ps.setString(3, post.getAuthor());
            ps.executeUpdate();

        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     * 查询所有帖子，按创建时间倒序
     */
    public List<ForumPost> queryAllPosts() {
        List<ForumPost> list = new ArrayList<>();
        String sql = "SELECT * FROM forum_post ORDER BY create_time DESC";
        try (Connection conn = DBUtil.getConnection();
             PreparedStatement ps = conn.prepareStatement(sql)) {

            ResultSet rs = ps.executeQuery();
            while (rs.next()) {
                ForumPost fp = new ForumPost();
                fp.setId(rs.getInt("id"));
                fp.setTitle(rs.getString("title"));
                fp.setContent(rs.getString("content"));
                fp.setAuthor(rs.getString("author"));
                fp.setCreateTime(rs.getTimestamp("create_time"));
                list.add(fp);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return list;
    }
}

