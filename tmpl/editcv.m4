include(`tmpl/partials/doc_open.m4')
<main>
	<form action="/editcv" method="post">
		<label>
			Content:
			<textarea name="content">@@content@@</textarea>
		</label>
		<br>
		<button>
			Submit
		</button>
	</form>
</main>
include(`tmpl/partials/doc_close.m4')
