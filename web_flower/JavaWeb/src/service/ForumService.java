package service;


import dao.ForumPostDao;
import model.ForumPost;

import java.util.List;

/**
 * ForumService - 处理论坛帖子相关的业务逻辑
 */
public class ForumService {

    private ForumPostDao forumPostDao = new ForumPostDao();

    /**
     * 创建新帖子
     */
    public void createPost(ForumPost post) {
        forumPostDao.insertPost(post);
    }

    /**
     * 获取所有帖子列表
     */
    public List<ForumPost> listAllPosts() {
        return forumPostDao.queryAllPosts();
    }
}
