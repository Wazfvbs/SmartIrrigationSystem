<%@ page contentType="text/html;charset=UTF-8" language="java" %>
<jsp:include page="/header.jsp"/>

<h2 class="mb-4">我的账户</h2>

<!-- 可以显示用户信息、编辑资料等 -->
<div class="row">
    <div class="col-md-4">
        <img src="images/user_avatar.png" class="img-fluid rounded" alt="头像">
    </div>
    <div class="col-md-8">
        <h5>用户名: 张三</h5>
        <p>邮箱: zhangsan@example.com</p>
        <p>签名: 爱生活，爱养花。</p>
        <button class="btn btn-outline-primary">编辑资料</button>
    </div>
</div>

<jsp:include page="/footer.jsp"/>
