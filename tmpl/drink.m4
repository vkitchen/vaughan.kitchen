include(`tmpl/partials/doc_open.m4')

<div class="page-nav">
	<form action="/cocktails/search" method="get" class="page-nav__searchbox">
		<input type="text" name="q">
		<button>search</button>
	</form>
	@@new-cocktail-link@@
</div>

<main>
	@@edit-cocktail-link@@
	<h3>@@drink-title@@</h3>
	<div class="drink__img">
		<a href="@@drink-href@@" class="drink__recipe-link">
			<img src="@@drink-img@@">
		</a>
	</div><!--
	--><div class="drink__recipe">
		<p>Drinkware: @@drink-drinkware@@</p>
		<p>Serve: @@drink-serve@@</p>
		<p>Garnish: @@drink-garnish@@</p>
		<ul class="ingredients-list">
			@@drink-ingredients@@
		</ul>
		<p>@@drink-method@@</p>
	</div>
</main>

include(`tmpl/partials/doc_close.m4')
