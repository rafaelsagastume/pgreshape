# pgreshape
Embed the new column according to the desired position in any table in the postgresql database.

### Why pgreshape?
It helps you to generate an sql script that looks for the dependencies of the table to be able to embed the new column, these dependencies are found recursively :-) Then recreate the table supporting you in the reconstruction of the original structure.

```
 ----                                    ----
| id |                                  | id |
 ----                                    ----
 ----                                    ----
| c1 |                                  | c1 |
 ----                                    ----
 ----                ----                ----
| c2 |              |newc|              |newc|
 ----                ----                ----
 ----     -> pgreshape (file.sql) ->     ----
| c3 |          offset = c1             | c2 |
 ----                                    ----
 ----                                    ----
| c4 |                                  | c3 |
 ----                                    ----
                                         ----
                                        | c4 |
                                         ----
```

### support:
<table>
	<thead>
    <tr>
			<td colspan="3" align="center">
        <strong>Table dependencies</strong>
			</td>
		</tr>
		<tr>
			<td><strong>Object</strong></td>
			<td><strong>Support</strong></td>
			<td><strong>Comments</strong></td>
		</tr>
	</thead>
	<tbody>
		<tr>
			<td>Columns</td>
			<td>partial</td>
			<td></td>
		</tr>
		<tr>
			<td>ACL on Columns</td>
			<td>not implemented</td>
			<td></td>
		</tr>
		<tr>
			<td>Comment</td>
			<td>complete</td>
			<td></td>
		</tr>
		<tr>
			<td>Foreign Key</td>
			<td>complete</td>
			<td></td>
		</tr>
		<tr>
			<td>Index</td>
			<td>complete</td>
			<td></td>
		</tr>
		<tr>
			<td>Unique</td>
			<td>complete</td>
			<td></td>
		</tr>
		<tr>
			<td>Views recursive</td>
			<td>complete</td>
			<td></td>
		</tr>
		<tr>
			<td>ACL on View</td>
			<td>complete</td>
			<td></td>
		</tr>
	</tbody>
</table>


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
