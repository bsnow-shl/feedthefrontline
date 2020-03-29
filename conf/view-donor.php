<?php
/**
 * Copyright 2018 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
// [START calendar_quickstart]
require __DIR__ . '/vendor/autoload.php';
include 'getclient.php';

if (php_sapi_name() != 'cli') {
    throw new Exception('This application must be run on the command line.');
}


/*

Viewing the donor page with no context should show:
-a list of hospitals in a drop-down
-calendar items for the selected calendar (wouldn't a calendar widget in js be nice?)

*/




// Get the API client and construct the service object.
$client = getClient();
$service = new Google_Service_Calendar($client);

print "Calendars:\n";
printf("%v\n",$service->calendarList->listCalendarList());
foreach ($service->calendarList as $cal) {
    printf("%s %s \n", $cal->calendar_id, $cal->title);
}

