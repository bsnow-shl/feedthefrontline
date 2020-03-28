create sequence admin_areas_id start 100;

create table admin_categories (
	admin_category_id integer not null primary key,
	name text not null,
	order_in_page integer not null default 100
);

create table admin_areas (
	admin_area_id integer not null primary key default nextval('admin_areas_id'),
	admin_category_id integer references admin_categories,
	order_in_category integer not null default 100,
	role_id integer not null references roles,

	-- either provide both of these
	uri text ,
	description text unique ,
	caption text,
	
	-- or both of these	
	include_header text ,
	function text,


	check (   (uri is not null and description is not null) or (include_header is not null and function is not null) )

);



insert into roles (role_name,visible) values ('user_admin',1);
insert into roles (role_name,visible) values ('role_admin',1);
insert into roles (role_name,visible) values ('db_admin',1);

insert into admin_areas(uri,role_id,description, include_header, function, order_in_category) 
	select null, role_id, null , 'users.h', 'user_render_adminmenu' , 90 from roles where role_name='user_admin';


insert into admin_areas(uri,role_id,description, include_header, function, order_in_category)
	select '/admin/users/', role_id, 'Users' , null, null, 91 from roles where role_name='user_admin';

insert into admin_areas(uri,role_id,description, include_header, function, order_in_category)
	select '/admin/users/reports/', role_id, 'User reports' , null, null, 91 from roles where role_name='user_admin';

insert into admin_areas(uri,role_id,description) 
	select '/admin/db/', role_id, 'Database admin' from roles where role_name='db_admin';


insert into admin_categories(name,order_in_page, admin_category_id) values ('Site content',101, 1);
insert into admin_categories(name,order_in_page, admin_category_id) values ('Site health', 100, 2);
insert into admin_categories(name,order_in_page, admin_category_id) values ('Back-end',200, 3);
