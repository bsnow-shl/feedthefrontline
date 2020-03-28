create table uploads (
    upload_id integer not null primary key,

    referring_table text,
    referring_id integer,

    original_name text not null,
    local_filename text not null,

    -- some computed fields
    mime_type text not null,
    size integer not null,
    image_width integer,
    image_height integer
);
