include(`tmpl/partials/doc_open.m4')

<main>
	<form action="@@edit-cocktail-path@@" method="post">
		<label>
			Title:
			<input type="text" name="title" value="@@drink-title@@">
		</label>
		<br>
		<label>
			Serve:
			<input type="text" name="serve" value="@@drink-serve@@">
		</label>
		<br>
		<label>
			Garnish:
			<input type="text" name="garnish" value="@@drink-garnish@@">
		</label>
		<br>
		<label>
			Drinkware:
			<input type="text" name="drinkware" value="@@drink-drinkware@@">
		</label>
		<br>
		<label>
			Method:
			<textarea name="method">@@drink-method@@</textarea>
		</label>
		<br>
		<button>
			Submit
		</button>
	</form>
</main>

include(`tmpl/partials/doc_close.m4')
