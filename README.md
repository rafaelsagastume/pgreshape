# pgreshape
Embed the new column according to the desired position in any table in the postgresql database.

### compile:
```
$ git clone https://github.com/rafaelsagastume/pgreshape.git
$ cd pgquarrel
$ cmake .
$ make
```

### reshape.conf:
```
host = localhost
port = 5432
dbname = db1
user = postgres
password = 123
file =
```

### example:
> `pgreshape -c /opt/reshape.conf -s rrhh -t expediente -offset emisor -column nueva -type 'boolean'`
