<?

function tickets_render_adminmenu() {
	$email = user_email();
	$count = db_value('select count(*) from tickets join ticket_component_cc using (ticket_component_id) where ticket_status=? and email=?', 'open', $email);
	if ($count) {
		print "<a href=\"/admin/tickets/\">$count tickets</a> open for you.<br/>";
	}
}

?>
