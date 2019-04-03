from cosmosTroposphere import CosmosTemplate
t = CosmosTemplate(component_name="apps.newslabs.tools.bbc.co.uk", description="Generic secure front-end proxy to S3 apps.")
print(t.to_json())
