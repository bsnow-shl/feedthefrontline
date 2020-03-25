<?php

function calendar_render($year, $month, $callback='', $callback_parameter = null)  {
    $unix = mktime(0,0,0,$month,1,$year);
    $info = getdate($unix);
    $of_week = $info[wday];

    $in_month = mcal_days_in_month($month, mcal_is_leap_year($year));

    print '<table class="calendar">';
    print '<tr class="calendarhead"><th class="calendarheadcell">sun</th><th class="calendarheadcell">mon</th><th class="calendarheadcell">tue</th><th class="calendarheadcell">wed</th><th class="calendarheadcell">thu</th><th class="calendarheadcell">fri</th><th class="calendarheadcell">sat</th></tr><tr>';

    $tr_printed = 1;
    print str_repeat('<td class="calendarcell">&nbsp;</td>', $of_week);

    for ($i=1; $i<=$in_month; $i++) {
        if (!$tr_printed) {
            print '<tr class="calendaryrow">';
            $tr_printed = 1;
        }
        if ($callback) {
            $text = $callback($year, $month, $i, $callback_parameter);
            if (is_array($text)) {
                $link = $text[link];
                $text = $text[text];
            }
        }

        if ($link) {
            print "<td class=\"calendarcell\"><a class=\"calendarlink\" href=\"$link\">$i</a><div class=\"calendarbody\">$text</div></td>";
        } else {
            print "<td class=\"calendarcell\">$i<div class=\"calendarbody\">$text</div></td>";
        }

        $of_week++;
        if ($of_week==7) {
            print '</tr>';
            $of_week=0;
            $tr_printed=0;
        }
    }
    if ($tr_printed) {
        print '</tr>';
    }
    print '</table>';
}
?>

