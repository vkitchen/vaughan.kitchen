include(`tmpl/partials/doc_open.m4')

<div class="page-nav">
	<form action="/cocktails/search" method="get" class="page-nav__searchbox">
		<input type="text" name="q">
		<button>search</button>
	</form>
	@@new-cocktail-link@@
</div>

<main>
	<h3>@@title@@</h3>
	<ul class="drinks-list">
		@@drinks@@
	</ul>
	@@next-page-link@@
</main>
include(`tmpl/partials/doc_close.m4')
