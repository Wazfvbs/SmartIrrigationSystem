<%@ page contentType="text/html;charset=UTF-8" language="java" %>
<jsp:include page="/header.jsp"/>

<h2 class="mb-4">养花指南</h2>

<!-- 图文示例 -->
<div class="row">
    <div class="col-md-4">
        <img src="images/guide.jpg" class="img-fluid rounded shadow" alt="guide">
    </div>
    <div class="col-md-8">
        <p class="fs-5">
            养花需要根据花的种类、习性来选择合适的土壤和环境。
            不同的植物对水分、光照、温度的需求并不相同。<br/>
            建议：
        <ul>
            <li>按时浇水，但避免过度</li>
            <li>保持土壤透气</li>
            <li>定期施肥</li>
            <li>防止病虫害</li>
        </ul>
        </p>
    </div>
</div>

<!-- 更多内容可自行添加 -->

<jsp:include page="/footer.jsp"/>
