<%@ page contentType="text/html;charset=UTF-8" language="java" %>
<jsp:include page="/header.jsp"/>

<h2 class="mb-4">我的花</h2>

<!-- 这里可以展示用户添加的花盆/植物列表 -->
<div class="row row-cols-1 row-cols-md-2 g-4">
    <div class="col">
        <div class="card h-100">
            <img src="images/flower.jpg" class="card-img-top" alt="我的花">
            <div class="card-body">
                <h5 class="card-title">玫瑰</h5>
                <p class="card-text">这是我最喜欢的玫瑰花，每天浇水一次...</p>
            </div>
            <div class="card-footer">
                <small class="text-muted">上次浇水：今天 8:30</small>
            </div>
        </div>
    </div>
    <div class="col">
        <div class="card h-100">
            <img src="images/flower2.jpg" class="card-img-top" alt="我的花2">
            <div class="card-body">
                <h5 class="card-title">多肉植物</h5>
                <p class="card-text">可爱的小多肉，注意不要浇水过多...</p>
            </div>
            <div class="card-footer">
                <small class="text-muted">上次浇水：昨天 17:20</small>
            </div>
        </div>
    </div>
</div>

<jsp:include page="/footer.jsp"/>
