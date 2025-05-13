<%@ page contentType="text/html;charset=UTF-8" language="java" %>
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>智慧花盆</title>
    <!-- 引入 Bootstrap 5 CDN -->
    <link rel="stylesheet"
          href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css">
    <!-- 自定义样式 -->
    <link rel="stylesheet" href="css/style.css">

    <!-- 可选字体图标 (Bootstrap Icons) -->
    <link rel="stylesheet"
          href="https://cdn.jsdelivr.net/npm/bootstrap-icons@1.10.5/font/bootstrap-icons.css">

</head>
<body>
<!-- 导航栏 -->
<nav class="navbar navbar-expand-lg navbar-dark bg-dark">
    <div class="container-fluid">
        <a class="navbar-brand" href="index.jsp">智慧花盆</a>
        <button class="navbar-toggler" type="button" data-bs-toggle="collapse"
                data-bs-target="#navbarNav" aria-controls="navbarNav"
                aria-expanded="false" aria-label="切换导航">
            <span class="navbar-toggler-icon"></span>
        </button>
        <div class="collapse navbar-collapse" id="navbarNav">
            <ul class="navbar-nav ms-auto">
                <!-- 首页 -->
                <li class="nav-item">
                    <a class="nav-link" href="index.jsp">首页</a>
                </li>
                <!-- 我的花 -->
                <li class="nav-item">
                    <a class="nav-link" href="myFlower.jsp">我的花</a>
                </li>
                <!-- 养花指南 -->
                <li class="nav-item">
                    <a class="nav-link" href="guide.jsp">养花指南</a>
                </li>
                <!-- 论坛 -->
                <li class="nav-item">
                    <a class="nav-link" href="Forum">论坛</a>
                </li>
                <!-- 我的 -->
                <li class="nav-item">
                    <a class="nav-link" href="myPage.jsp">我的</a>
                </li>
            </ul>
        </div>
    </div>
</nav>
<!-- 容器开始 -->
<div class="container mt-3">
