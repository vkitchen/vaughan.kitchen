include(`tmpl/partials/doc_open.m4')
<br>
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
include(`tmpl/partials/doc_close.m4')
