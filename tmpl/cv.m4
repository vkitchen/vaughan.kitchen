include(`tmpl/partials/doc_open.m4')
<main>
	@@edit-cv-link@@
	<br>
	<p>Last updated: @@mtime@@</p>
	@@content@@
</main>
include(`tmpl/partials/doc_close.m4')
