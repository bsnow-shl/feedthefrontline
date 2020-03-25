<?

function _translation_get_dir() {
    return $_SERVER['DOCUMENT_ROOT'].'/../conf/locale/';
}

function _translation_get_translation_file($txn) {
    $file = _translation_get_dir().'/'.$txn.'/LC_MESSAGES/messages.po';
    if (!file_exists($file)) {
        error_log("translation found that this file does not exist: $file");
        return false;
    }
    if (!is_writable($file)) {
        error_log("translation found that this file is not writeable: $file");
        return false;
    }
    return $file;
}

function translation_get_translations() {
    $list = array();
    $dh = opendir($dir = _translation_get_dir());
    
    while ($fn = readdir($dh)) {
        if ($fn=='.' || $fn=='..' || ! is_dir($dir.$fn)) { continue; }

        if (file_exists($dir.$fn.'/LC_MESSAGES/messages.po') && 
            is_writable($dir.$fn.'/LC_MESSAGES/messages.po') ) {
            $list[] = $fn;
        }
    }
    return $list;
}

function _translation_rebuild() {
    $dir = _translation_get_dir();
    chdir($dir);
    return `make  2>&1`;
}

function translation_get_mappings($txn) {
    $file = _translation_get_translation_file($txn);
    if (!$file) return FALSE;

    $fh = fopen ($file,'r');
    if (!$fh) return FALSE;

    $txlist = array();

    // 0 = looking for msgid
    // 1 = reading msgid
    // 2 = reading msgstr
    $mode = 0;
    while ($line = fgets($fh, 40960)) {
        switch ($mode) {
            case 0:
                if (substr($line,0,5)=='msgid') {
                    $source_line = $dest_line = '';
                    $pos = strpos($line, '"');
                    $source_line .= substr($line, $pos+1, strlen($line)-$pos-strpos('"', $line)-3);
                    $mode++;
                }
                break;

            case 1:
                if (substr($line,0,6)=='msgstr') {
                    $pos = strpos($line, '"');
                    $dest_line .= substr($line, $pos+1, strlen($line)-$pos-strpos('"', $line)-3);
                    $mode++;
                } else {
                    $pos = strpos($line, '"');
                    $source_line .= substr($line, $pos+1, strlen($line)-$pos-strpos('"', $line)-3);
                }
                break;

            case 2: 
               if (substr($line,0,1)=='#' || strlen($line)==1) {
                    # fixups
                    $source_line = str_replace('\n', "\n", $source_line);
                    $dest_line = str_replace('\n', "\n", $dest_line);
                    $source_line = str_replace('\"', '"', $source_line);
                    $dest_line = str_replace('\"', '"', $dest_line);
                    $txnlist[] = array($source_line,$dest_line);
                    $mode=0;
               } else {
                    $pos = strpos($line, '"');
                    $dest_line .= substr($line, $pos+1, strlen($line)-$pos-strpos('"', $line)-3);
               }
               break;
        }
    }

    return $txnlist;
}


function translation_save_translations($txn, $what) {
    $file = _translation_get_translation_file($txn);
    if (!$file) {
        $file = tempnam('/tmp/',config_get('site', 'short_name').'translate');
        $warning = "<p><b>Created temporary file $file instead for $txn; please notify support of this.</b></p>";
        error_log($warning);
    }

    $fh = fopen ($file,'w');
    if (!$fh) return array (FALSE, "Failed to open for write");

    foreach ($what as $w) {
        fwrite($fh, sprintf("msgid \"%s\"\nmsgstr \"%s\"\n\n", 
               str_replace("\r", '', str_replace("\n", '\n', str_replace('"', '\"', $w[0]))), 
               str_replace("\r", '', str_replace("\n", '\n', str_replace('"', '\"', $w[1])))));

    }
    fclose($fh);

    $result = _translation_rebuild();

    return array(true, "$warning <p>Result of make:<pre>".$result.'</pre>');
}

?>
