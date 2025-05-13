package servlet;

import model.ForumPost;
import service.ForumService;

import javax.servlet.ServletException;
import javax.servlet.http.*;
import java.io.IOException;
import java.util.List;

/**
 * ForumServlet - 处理论坛发帖、查看帖子列表
 * 路径: /Forum
 *
 * GET /Forum => 显示帖子列表
 * POST /Forum => 发帖
 */
public class ForumServlet extends HttpServlet {

    private ForumService forumService = new ForumService();

    @Override
    protected void doGet(HttpServletRequest req, HttpServletResponse resp)
            throws ServletException, IOException {
        // 查询帖子列表
        List<ForumPost> postList = forumService.listAllPosts();

        // 放到 request 范围
        req.setAttribute("postList", postList);

        // 跳转到 forum.jsp
        req.getRequestDispatcher("/forum.jsp").forward(req, resp);
    }

    @Override
    protected void doPost(HttpServletRequest req, HttpServletResponse resp)
            throws ServletException, IOException {
        req.setCharacterEncoding("UTF-8");

        String title = req.getParameter("title");
        String content = req.getParameter("content");
        String author = req.getParameter("author");

        ForumPost post = new ForumPost();
        post.setTitle(title);
        post.setContent(content);
        post.setAuthor(author);

        // 保存帖子
        forumService.createPost(post);

        // 发帖完成后，重定向回帖子列表
        resp.sendRedirect("Forum");
    }
}

