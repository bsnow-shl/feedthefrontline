create sequence dw_fact_id start 100;
create sequence dw_fact_number_id start 100;
create sequence dw_dimension_id start 100;
create sequence dw_attribute_id start 100;
drop table dw_attributes;
drop table dw_dimensions;
drop table dw_fact_numbers;
drop table dw_facts;



-- facts we are tracking, e.g. "sales"
create table dw_facts (
	fact_id integer not null primary key default nextval('dw_fact_id'),

	pretty_name text unique not null,
	table_name text unique not null,

	fact_include_file text
);

-- numbers about those fact that we are interested in, e.g. "price", "time to close the order"
-- these should be numeric, continuously valued, and additive if you ask philg
-- these are numbers that get sum(), avg() and go in HAVING clauses
create table dw_fact_numbers (
	fact_number_id integer not null primary key default nextval('dw_fact_number_id'),

	fact_id integer not null references dw_facts,

	pretty_name text not null,
	column_name text not null,
	column_sql_type text not null
);

-- these can be used to name interesting dimensions about the fact
-- i.e. time, purchaser, recipient, product characteristics
-- dimension attributes are WHEREd, and GROUP BYed
create table dw_dimensions (
	dimension_id integer not null primary key default nextval('dw_dimension_id'),

	fact_id integer not null references dw_facts,
	
	pretty_name text not null,
	table_name text not null,

	loader_header text,
	loader_function text,

	unique (fact_id, table_name)
);

-- specific fields on the dimensions. e.g. for time: which holiday season does the day fall in?
create table dw_attributes (
	attribute_id integer not null primary key default nextval('dw_attribute_id'),
	dimension_id integer not null references dw_dimensions,

	is_identifying integer not null default 0,

	pretty_name text not null,		
	column_name text not null,
	column_sql_type text not null,

	display_function text,	-- this could turn a day-of-week # into a string like Monday
	menu_function text,     -- list of multiple-choices

	unique(dimension_id, column_name)
);




-- a sale is a Product bought by a Buyer at a Price on a given Date
insert into dw_facts (pretty_name, table_name) values ('Sales', 'sales');
insert into dw_fact_numbers(fact_id, pretty_name, column_name,column_sql_type) 
		select fact_id, 'Order subtotal', 'order_subtotal', 'numeric(8,2)' from dw_facts where table_name='sales';
insert into dw_fact_numbers(fact_id, pretty_name, column_name,column_sql_type) 
		select fact_id, 'Taxes', 'taxes', 'numeric(8,2)' from dw_facts where table_name='sales';
insert into dw_fact_numbers(fact_id, pretty_name, column_name,column_sql_type) 
		select fact_id, 'Shipping & delivery', 'shipping', 'numeric(8,2)' from dw_facts where table_name='sales';
insert into dw_fact_numbers(fact_id, pretty_name, column_name,column_sql_type) 
		select fact_id, 'Order total', 'order_total', 'numeric(8,2)' from dw_facts where table_name='sales';
insert into dw_fact_numbers(fact_id, pretty_name, column_name,column_sql_type) 
		select fact_id, 'Days until delivery', 'days_to_delivery', 'integer' from dw_facts where table_name='sales';


insert into dw_dimensions (fact_id, pretty_name, table_name) 
(		select fact_id, 'Product', 'product' from dw_facts where table_name='sales');
insert into dw_dimensions (fact_id, pretty_name, table_name, loader_header, loader_function) 
(		select fact_id, 'Buyer address', 'buyer', 'dw.h', 'dw_load_buyer_dimension' from dw_facts where table_name='sales');
insert into dw_dimensions (fact_id, pretty_name, table_name, loader_header, loader_function) 
(		select fact_id, 'Recipient address', 'recipient', 'dw.h', 'dw_load_recipient_dimension' from dw_facts where table_name='sales');
insert into dw_dimensions (fact_id, pretty_name, table_name, loader_header, loader_function) 
(		select fact_id, 'Date', 'date', 'dw.h', 'dw_load_date_dimension' from dw_facts where table_name='sales');
insert into dw_dimensions (fact_id, pretty_name, table_name, loader_header, loader_function) 
(		select fact_id, 'Price', 'price', 'dw.h', 'dw_load_price_dimension' from dw_facts where table_name='sales');


-- product dimension: pkey = code
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 1, 'Code', 'code', 'integer', null from dw_dimensions where table_name='product');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'Name', 'name', 'text', null from dw_dimensions where table_name='product');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'Luxury Product', 'is_luxury', 'integer', null from dw_dimensions where table_name='product');

-- buyer dimension: pkey = userid

insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 1, 'Country', 'country', 'text', null from dw_dimensions where table_name='buyer');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 1, 'Postal Code', 'postal_code', 'text', null from dw_dimensions where table_name='buyer');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'Region', 'region', 'text', null from dw_dimensions where table_name='buyer');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'Province', 'state', 'text', null from dw_dimensions where table_name='buyer');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'County', 'county', 'text', null from dw_dimensions where table_name='buyer');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'City', 'city', 'text', null from dw_dimensions where table_name='buyer');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'Longtitude', 'longitude', 'numeric(15,12)', null from dw_dimensions where table_name='buyer');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'Latitude', 'latitude', 'numeric(15,12)', null from dw_dimensions where table_name='buyer');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'Average household size', 'average_household_size', 'numeric(6,2)', null from dw_dimensions where table_name='buyer');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'Average travel time to work', 'average_travel_time_to_work', 'numeric(6,2)', null from dw_dimensions where table_name='buyer');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'Median household income', 'median_household_income', 'integer', null from dw_dimensions where table_name='buyer');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'Percent of families below poverty level', 'percent_family_below_poverty_level', 'numeric(6,2)', null from dw_dimensions where table_name='buyer');

insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'Housing units total', 'housing_units_total', 'integer', null from dw_dimensions where table_name='buyer');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'Income per capita', 'income_per_capita', 'integer', null from dw_dimensions where table_name='buyer');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'Median home value', 'median_home_value', 'integer', null from dw_dimensions where table_name='buyer');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'Percent vacant housing units', 'percent_vacant_housing_units', 'numeric(6,2)', null from dw_dimensions where table_name='buyer');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'Percent of housing units owner occupied', 'percent_housing_unit_owner_occupied', 'numeric(6,2)', null from dw_dimensions where table_name='buyer');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'Percent of housing units renter occupied', 'percent_housing_unit_renter_occupied', 'numeric(6,2)', null from dw_dimensions where table_name='buyer');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'Percent of housing units occupied', 'percent_housing_units_occupied', 'numeric(6,2)', null from dw_dimensions where table_name='buyer');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'Percent population of several races', 'percent_population_several_races', 'numeric(6,2)', null from dw_dimensions where table_name='buyer');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'Perecent population pacific/Hawaiian origin', 'percent_population_pacific', 'numeric(6,2)', null from dw_dimensions where table_name='buyer');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'Percent population other race', 'percent_population_other_race', 'numeric(6,2)', null from dw_dimensions where table_name='buyer');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'Percent population asian', 'percent_population_asian', 'numeric(6,2)', null from dw_dimensions where table_name='buyer');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'Percent population black', 'percent_population_black', 'numeric(6,2)', null from dw_dimensions where table_name='buyer');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'Percent population white', 'percent_population_white', 'numeric(6,2)', null from dw_dimensions where table_name='buyer');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'Percent population latino', 'percent_population_latino', 'numeric(6,2)', null from dw_dimensions where table_name='buyer');

insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'Percent speak language other than English', 'percent_speak_non_english', 'numeric(6,2)', null from dw_dimensions where table_name='buyer');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'Percent of population that are veterans', 'percent_veterans', 'numeric(6,2)', null from dw_dimensions where table_name='buyer');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'Percent completed high school', 'percent_completed_high_school', 'numeric(6,2)', null from dw_dimensions where table_name='buyer');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'Percent completed bachelor', 'percent_completed_bachelor', 'numeric(6,2)', null from dw_dimensions where table_name='buyer');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'Percent that are disabled', 'percent_disabled', 'numeric(6,2)', null from dw_dimensions where table_name='buyer');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'Percent of population foreign born', 'percent_foreign_born', 'numeric(6,2)', null from dw_dimensions where table_name='buyer');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'Percent of population that are married', 'percent_married', 'numeric(6,2)', null from dw_dimensions where table_name='buyer');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'Percent of population in labour force', 'percent_in_labour_force', 'numeric(6,2)', null from dw_dimensions where table_name='buyer');

-- recipient dimension:
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)
	select (select dimension_id from dw_dimensions where table_name='recipient'), is_identifying, pretty_name, column_name, column_sql_type, display_function from dw_attributes where dimension_id=(select dimension_id from dw_dimensions where table_name='buyer');


-- date dimensions: identified by y-m-d
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 1, 'Date',  'pgsql_date', 'date', null from dw_dimensions where table_name='date');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'day of month', 'mday', 'integer', null from dw_dimensions where table_name='date');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'month of year', 'mon', 'integer', null from dw_dimensions where table_name='date');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'year', 'year', 'integer', null from dw_dimensions where table_name='date');

insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function,menu_function)  
(	select dimension_id, 0, 'day of week', 'dow', 'integer', 'dw_day_of_week_render', 'dw_day_of_week_list' from dw_dimensions where table_name='date');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'day number overall', 'doverall', 'integer', null from dw_dimensions where table_name='date');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'week in year', 'week_in_year', 'integer', null from dw_dimensions where table_name='date');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'week in month', 'week_in_month', 'integer', null from dw_dimensions where table_name='date');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'week number overall', 'week_number_overall', 'integer', null from dw_dimensions where table_name='date');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'month number overall', 'month_number_overall', 'integer', null from dw_dimensions where table_name='date');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'quarter', 'quarter', 'integer', null from dw_dimensions where table_name='date');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'days to holiday', 'days_until', 'text', null from dw_dimensions where table_name='date');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function,menu_function)  
(	select dimension_id, 0, 'holiday season', 'holiday_season', 'text', 'dw_holiday_render', 'dw_holiday_menu' from dw_dimensions where table_name='date');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function,menu_function)  
(	select dimension_id, 0, 'Weekday', 'is_weekday', 'integer', 'dw_flag_render', 'dw_flag_menu' from dw_dimensions where table_name='date');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function,menu_function)  
(	select dimension_id, 0, 'Holiday', 'is_holiday', 'integer', 'dw_flag_render', 'dw_flag_menu' from dw_dimensions where table_name='date');

-- price dimension:  pkey = dollar value
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 1, 'Price', 'price', 'numeric(8,2)', null from dw_dimensions where table_name='price');
insert into dw_attributes(dimension_id, is_identifying, pretty_name, column_name, column_sql_type, display_function)  
(	select dimension_id, 0, 'Price Band', 'price_band', 'text', 'dw_price_band' from dw_dimensions where table_name='price');

