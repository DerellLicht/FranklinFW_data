# Replacing the Monday iOS export with the FranklinWH cloud API (findings, 2026-11-21)

## Verdict
Feasible. FrankEnPoller already logs in to the same host/token the energy-history endpoint needs.
Its FranklinCloudClient only uses the live endpoint (getDeviceCompositeInfo), which gives *today's*
totals only (reset at midnight) - no history. History lives in a second endpoint, not yet used by the app.

## Endpoint
GET https://energy.franklinwh.com/api-energy/electric/getFhpElectricData
Header: loginToken (same token as FranklinCloudClient.LoginAsync)
Params: gatewayId, type=2 (week), startDate=<Monday YYYY-MM-DD>   (server returns empty arrays unless the date is a Monday)
type: 1 day (param is dayTime, not startDate), 2 week, 3 month, 4 year, 5 lifetime.
Source: franklinwh-cloud (david2069) docs/source, const/energy_fields.py. Unofficial/undocumented.

## CSV column -> response array
Home (kWh)                 kwhLoadArray
Solar (kWh)                kwhSuArray
aPower(s) Charge (kWh)     kwhFhpChgArray
aPower(s) Discharge (kWh)  kwhFhpDiArray
Grid Import (kWh)          kwhUtiInArray
Grid Export (kWh)          kwhUtiOutArray
Generator (kWh)            kwhGenArray
V2L (kWh)                  kwhV2lArray
Date                       deviceTimeArray (bucket label)

## Not yet verified
- No live call made (no credentials in the session). First run should compare against a real app export.
- Week response is expected to hold 7 daily buckets; inferred from deviceTimeArray docs.
- Rounding: CSV has 1 decimal; API precision unknown.
- Sample CSV (Nov 9-15 2026) sanity check: solar + import - export + discharge - charge = home within 0.1 kWh every day.
