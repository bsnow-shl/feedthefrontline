<?

/***** documentation *****


Here's a regular table presentation, no pages:

http://testmachine/test.html

<table>
<tr><th>x</th><th>y</th><th>z</th></tr>
<? foreach (db_multirow("select x,y,z from tabletable") as $row) : ?>
  <tr><td><%=$row['x']%></td><td><%=$row['y']%></td><td><%=$row['z']%></td></tr>
<? endforeach ?>
</table>


If you have a lot of data, and you don't want it all
in one page, use this instead:


http://testmachine/test.html

<? $sqlfragment = "from tabletable"
   $list = db_paged_multirow("select x,y,z $sqlfragment");
   $count = db_value("select count(1) $sqlfragment"); ?>
<table>
<tr><th>x</th><th>y</th><th>z</th></tr>
<? foreach ($list as $row) : ?>
  <tr><td><%=$row['x']%></td><td><%=$row['y']%></td><td><%=$row['z']%></td></tr>
<? endforeach ?>
</table>
<%=page_prev_href($count)%> 
<%=page_resultcount($count)%> 
<%=page_next_href($count)%>


And if you don't like the default of 9 items per page (think tic-tac-toe
merchandise presentation), you can change the count in the URL:
http://testmachine/test.html?query_limit=30

Or set $_REQUEST['query_limit']=30; internally if you don't want to 
see it in the URL.

**************************/



/* Internal calls only. 
  check the request parameters, default them if necessary:
      offset=0
      limit=9.
  Use list($limit,$offset) = _page_getdefaults(); */
function _page_getdefaults() {
  $limit=9;
  $offset=0;
  if (isset($_REQUEST['query_limit'])) {
    $limit = intval($_REQUEST['query_limit']);
  }
  if (isset($_REQUEST['query_offset'])) {
    $offset = intval($_REQUEST['query_offset']);
  }
  return array($limit,$offset);
}

/* Internal calls only. 
   $op is a page-motion-factor.  1=>next page,
     -1=>previous page, 2=>fwd 2 pages, 0=>stay here,
     'walnut'=>error, etc 
   $totalcount is the number of rows selected
*/
function _page_href($op,$totalcount) {
  list($limit,$offset) = _page_getdefaults();
  /*
    'query_offset'=offset+limit
    'query_limit'=limit
  */

  //bound and calculate the new query_offset
  $nextoffset = $offset + $op*$limit;
  if ($nextoffset < 0 || $nextoffset >= $totalcount) {
    //nextoffset is out of bounds
    return null;
  }

  //arrayify QUERY_STRING and replace the relevant bits:
  $params = array();
  parse_str($_SERVER['QUERY_STRING'],$params);
  $params['query_limit'] = $limit;
  $params['query_offset'] = $nextoffset;
  
  return href($PHP_SELF,$params);
}

/* Returns a <A> hyperlink that references PHP_SELF, to 
   be used with db_paged_multirow (or similar) to control
   paging of a large table of data.  Returns the empty
   string if the 'Next' link is invalid in the current
   context (i.e. already looking at the last page)
   $totalcount total number of rows, used to calculate
     when the 'Next' link does not appear
   $linktext is text inside the href.  This may be HTML markup
   $extra might be "class='pagingwidget'" */
function page_next_href($totalcount, $linktext='Next', $extra='') {
  $link = _page_href(1,$totalcount);
  if ($link) {
    return "<a $extra href='$link'>$linktext</a>";
  } else {
    return "";
  }
}

/* Returns a <A> hyperlink that references PHP_SELF, to 
   be used with db_paged_multirow (or similar) to control
   paging of a large table of data.  Returns the empty
   string if the 'Prev' link is invalid in the current
   context (i.e. already looking at the first page)
   $totalcount total number of rows
   $linktext is text inside the href.  This may be HTML markup
   $extra might be "class='pagingwidget'" */
function page_prev_href($totalcount, $linktext='Prev', $extra='') {
  $link = _page_href(-1,$totalcount);
  if ($link) {
    return "<a $extra href='$link'>$linktext</a>";
  } else {
    return "";
  }
}

/* Returns a phrase that indicates the current page, e.g. 
   "Page 1 of 33."
   $totalcount is the total number of rows in the query
   $format is a sprintf format string. */
function page_resultcount($totalcount, $format="Page %d of %d") {
  list($limit,$offset) = _page_getdefaults();
  return sprintf($format, 1+$offset/$limit, ceil($totalcount/$limit));
}

?>
