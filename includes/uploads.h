<?php

function upload_delete($id) {
	$local = db_value('select local_filename from uploads where upload_id=?', $id);
	if ($local) {
		chdir($_SERVER['DOCUMENT_ROOT']);
		if (file_exists($local)) {
			unlink($local);
		}
		db_send('delete from uploads where upload_id=?', $id);
	}
}

# the caller must make sure the user is authorized to get this upload
function upload_replay($id, $cacheable=false) {
	$upload = db_row('select * from uploads where upload_id=?', $id);
	if (!$upload) {
		die ("No such file upload #$id");
	}

	$major = substr($upload['mime_type'], 0, strpos($upload['mime_type'], '/'));
	header('Content-type: '.$upload['mime_type']);
	header('Content-length: '.$upload['size']);

	if ($cacheable) { 
		header("Pragma: ");
		header("Cache-Control: ");
	} else {
		// http://support.microsoft.com/default.aspx?scid=kb;en-us;316431
		header('Pragma: ');
		header('Cache-control: no-cache, must-revalidate');
	}

	if ($upload['mime_type'] != 'application/pdf' && $major!='text' && $major != 'image' && $major != 'video') {
		header('Content-Disposition: attachment; filename="'.$upload['original_name'].'"');
	} else {
		header('Content-Disposition: inline; filename="'.$upload['original_name'].'"');
	}

	$filename = $_SERVER['DOCUMENT_ROOT'].'/'.$upload['local_filename'];
	$stat = stat($filename);
	header("Last-Modified: ".gmstrftime ("%a, %d %b %Y %T %Z", $stat[9]));
	header('Expires: '.gmstrftime ("%a, %d %b %Y %T %Z", time() + 3600*24));

	readfile ($filename);
}


# simple function to make a 2-way connection between an upload and some other cpf object.
# upload_associate($upload_id, 'documents', $doc_id, 'document_id', 'upload_id')
# -- presumes the table documents has a document_id primary key and uploads held in upload_id
function upload_associate($upload_id, $assoc_table, $assoc_objid, $assoc_table_pkey='', $assoc_upload_key='upload_id') {
	db_send('update uploads set referring_table=?,referring_id=? where upload_id=?', $assoc_table, $assoc_objid, $upload_id);

	if ($assoc_table_pkey && $assoc_upload_key) {
		db_send("update $assoc_table set $assoc_upload_key=? where $assoc_table_pkey=?", $upload_id, $assoc_objid);
	}
}

?>
