create table photo_albums (
	album_id integer not null primary key,
	album_uri text unique not null,
    short_uri text unique ,

	parent_album_id integer
);
create index photo_albums_uri on photo_albums(album_uri);


create sequence photo_order_sequence start 1;

create table photos (
	photo_id integer not null primary key,

	album_id integer not null references photo_albums,
	album_order integer not null default nextval('photo_order_sequence'),
	large_uri text not null,       -- in addition to photo_albums.url
	medium_uri text not null,       -- in addition to photo_albums.url
	small_uri text not null,       -- in addition to photo_albums.url

	large_width integer,
	large_height integer,

	medium_width integer,
	medium_height integer,

	small_width integer,
	small_height integer,

	photo_title text,
	caption text
);
create index photos_album on photos(album_id);


create table photo_keywords (
	photo_id integer references photos,
	keyword text
);
create index photo_keywords_keyword on photo_keywords(keyword);


insert into comment_systems (comment_system_id, singular, plural, table_name, pkey_column, title_column) values
        (2, 'photograph', 'photographs', 'photos', 'photo_id', 'photo_title');
        


insert into roles (role_name) values ('photos_admin');

insert into admin_areas(uri,description,role_id) 
        select '/admin/photos/', 'Photography', role_id from roles where role_name='photos_admin';



