<%@ page contentType="text/html;charset=UTF-8" language="java" %>
<%@ page import="model.ForumPost" %>
<%@ page import="java.util.List" %>
<jsp:include page="/header.jsp"/>

<h2 class="mb-4">花友论坛</h2>

<%
  // 从 request 中获取帖子列表
  List<ForumPost> postList = (List<ForumPost>) request.getAttribute("postList");
  if(postList == null) {
    // 可能 ForumServlet 未将数据放进 request
    // 这里仅示例
    postList = java.util.Collections.emptyList();
  }
%>

<!-- 帖子列表 -->
<div class="list-group mb-5">
  <%
    for(ForumPost post : postList) {
  %>
  <div class="list-group-item">
    <h5><%= post.getTitle() %></h5>
    <p><%= post.getContent() %></p>
    <small class="text-muted">作者: <%= post.getAuthor() %> | 时间: <%= post.getCreateTime() %></small>
  </div>
  <%
    }
  %>
</div>

<!-- 发帖表单 -->
<div class="card">
  <div class="card-header">
    发帖
  </div>
  <div class="card-body">
    <form action="Forum" method="post">
      <div class="mb-3">
        <label class="form-label">标题</label>
        <input type="text" name="title" class="form-control" required/>
      </div>
      <div class="mb-3">
        <label class="form-label">内容</label>
        <textarea name="content" class="form-control" rows="4" required></textarea>
      </div>
      <div class="mb-3">
        <label class="form-label">作者</label>
        <input type="text" name="author" class="form-control" required/>
      </div>
      <button type="submit" class="btn btn-success">发表</button>
    </form>
  </div>
</div>

<jsp:include page="/footer.jsp"/>
