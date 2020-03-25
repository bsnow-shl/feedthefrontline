<?

function newsletter_editions() {
	return config_get('newsletter','editions');
}

function newsletter_strong_id($noise = 'random') {
     return strtr( substr( base64_encode(md5('random!ze'.getmicrotime().$noise)) ,0, 16), '+=/', '012');
}

function newsletter_check_system_lists() {
    $callback = config_get('newsletter', 'system_lists_callback');
    if (!$callback) return null;

    db_send('begin');
    $nl_id = db_value('select newsletter_id from newsletters where name=?', 'System lists');
    if (!$nl_id) {
        $nl_id = db_newid('newsletters');
        db_insert_hash('newsletters', array(
            'newsletter_id' => $nl_id,
            'name' => 'System lists'
        ));
    }


    $current_q = db_multirow('select user_id,parameter from newsletter_subscriptions where newsletter_id=? order by user_id', $nl_id);
    foreach ($current_q as $r) {
            $current[$r['parameter']][] = $r['user_id'];
    }

    $should = $callback();
    foreach ($should as $edition => $user_list) {
        if (!$current[$edition]) { $current[$edition] = array(); }
        $to_remove = array_diff($current[$edition], $user_list);
        $to_add = array_diff($user_list, $current[$edition]);
        foreach ($to_add as $uid) {
            db_insert_hash('newsletter_subscriptions', array(
                'newsletter_subscription_id' => db_newid('newsletter_subscriptions'),
                'user_id' => $uid, 
                'parameter' => $edition,
                'newsletter_id' => $nl_id
            ));
        }

        if ($to_remove) {
            $remove_sql = '';
            foreach ($to_remove as $uid) {
                $remove_sql .= ','.$uid;
            }
            db_send("update newsletter_subscriptions set parameter=? where newsletter_id=? and user_id in (null $remove_sql)", 'unsubscribed', $nl_id);
        }

        $rc[$edition] = array('added'=>sizeof($to_add), 'removed' => sizeof($to_remove));
    }
    db_send('commit');
    return $rc;
}

?>
