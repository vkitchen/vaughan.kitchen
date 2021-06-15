<!--START page.html-->
<!DOCTYPE HTML>
<html>
<head>
  <meta charset="UTF-8">
  <title>@@drink-title@@ - Vaughan.Kitchen</title>
  <link rel="stylesheet" type="text/css" href="/static/css/cocktails.css">
</head>
<body>
  <!--START frame.html-->
  <div class="page-frame">
    <nav class="masthead-container">
      <div class="logo-container">
        <a class="logo" href="/">VK</a>
      </div>
      <div>
        <form class="masthead-search" action="/cocktails/search" method="get">
          <button class="search-btn">
            <span class="material-icons search-btn-icon">search</span>
          </button>
          <div class="masthead-search-terms">
            <input class="masthead-search-term" type="text" name="q">
          </div>
        </form>
      </div>
    </nav>
    <!--START drink.html-->
    <div class="content">
      <div class="content-inner">
        <div class="content-title">
          <h3>@@drink-title@@</h3>
        </div>
        <div class="drink">
          <div class="drink-img">
            <img src="@@drink-img@@">
          </div>
          <div class="recipe">
            <p>Drinkware: @@drink-drinkware@@</p>
            <p>Serve: @@drink-serve@@</p>
            <p>Garnish: @@drink-garnish@@</p>
            <ul>
               @@drink-ingredients@@
            </ul>
            <p>@@drink-method@@</p>
          </div>
        </div>
      </div>
    </div>
    <!--END drink.html-->
  </div>
  <!--END frame.html-->
</body>
</html>
<!--END page.html-->