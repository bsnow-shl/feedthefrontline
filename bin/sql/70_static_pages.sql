create table static_content (
	page_id integer not null primary key,

	uri text unique not null,
    title text not null,
    author integer not null references users(user_id),
    comments_permitted integer default '1' check (comments_permitted in (1,0))
);
create index page_uri  on static_content(uri);


insert into comment_systems (comment_system_id, singular, plural, table_name, pkey_column, title_column) values
        (4, 'web page', 'web pages', 'static_content', 'page_id', 'title');
        
insert into roles (role_name) values ('static_content_admin');

insert into admin_areas(uri,description,role_id) 
        select '/admin/static-content/', 'Static content', role_id from roles where role_name='static_content_admin';



