# Highlighters

Syntax highlighting for the internal editor, compatible with dark themes.

![preview](https://i.imgur.com/TpHiIgf.png)

# Some other tweaks
[Settings in doublecmd.xml](http://doublecmd.github.io/doc/en/configxml.html)

## Log message colors
```xml
  <Colors>
...
    <LogWindow>
      <Info>12622689</Info>
      <Error>6383296</Error>
      <Success>9097847</Success>
    </LogWindow>
  </Colors>
```

## SyncDirs colors
```xml
  <SyncDirs>
...
    <Colors>
      <Left>9097847</Left>
      <Right>12622689</Right>
      <Unknown>6383296</Unknown>
    </Colors>
  </SyncDirs>
```

## Differ colors
```xml
  <Differ>
...
    <Colors>
      <Added>9097847</Added>
      <Deleted>6383296</Deleted>
      <Modified>12622689</Modified>
    </Colors>
  </Differ>
```

## Native tabstop headers
```xml
    <ColumnsView>
      <TitleStyle>2</TitleStyle>
...
    </ColumnsView>
```
