module Path = {
  [@module "path"] external join2: (string, string) => string = "join";
};

let currentDir = Utils.getDirname();

let () = PageBuilder.setOutputDir(Path.join2(currentDir, "../build"));

let pageIndex: PageBuilder.page = {
  component: <ExampleIndex />,
  moduleName: ExampleIndex.moduleName,
  modulePath: ExampleIndex.modulePath,
  slug: "index",
  path: ".",
};

let page1: PageBuilder.page = {
  component: <ExamplePage1 />,
  moduleName: ExamplePage1.moduleName,
  modulePath: ExamplePage1.modulePath,
  slug: "page1",
  path: "page1",
};

let () = PageBuilder.buildPage(pageIndex);

let () = PageBuilder.buildPage(page1);

let () = PageBuilder.buildJsonWithWebpackPages();

// PageBuilder.startWatcher();
