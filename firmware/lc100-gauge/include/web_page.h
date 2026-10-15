// Pagina web servită de ESP32. Fără resurse externe (telefonul e conectat
// doar la ESP32, fără internet).
#pragma once

static const char kIndexHtml[] = R"HTML(<!doctype html>
<html lang="ro"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>LC100 Gauge</title>
<style>
:root{--bg:#111;--card:#1c1c1c;--fg:#eee;--dim:#888;--ok:#2c6;--warn:#f90;--crit:#e33;--line:#333}
*{box-sizing:border-box}body{margin:0;font:15px system-ui,sans-serif;background:var(--bg);color:var(--fg)}
header{display:flex;gap:4px;padding:8px;position:sticky;top:0;background:#000;z-index:1}
header button{flex:1;padding:10px 4px;border:0;border-radius:6px;background:#222;color:var(--fg);font-size:14px}
header button.on{background:#345}
section{display:none;padding:12px 16px}section.on{display:block}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(140px,1fr));gap:10px}
.card{background:var(--card);border-radius:8px;padding:10px;border-left:5px solid var(--ok)}
.card.l1{border-color:var(--warn)}.card.l2{border-color:var(--crit);background:#300}
.card .k{color:var(--dim);font-size:12px}.card .v{font-size:28px;font-variant-numeric:tabular-nums}
.card .f{color:var(--warn);font-size:12px;min-height:14px}
.meta{color:var(--dim);font-size:13px;margin-top:10px}
h3{margin:18px 0 6px;font-size:15px;color:#9cf}
table{width:100%;border-collapse:collapse}td,th{padding:4px;border-bottom:1px solid var(--line);text-align:left;font-size:13px}
input,select{width:100%;padding:6px;background:#000;color:var(--fg);border:1px solid var(--line);border-radius:4px;font-size:14px}
input[type=checkbox]{width:auto}
label{display:block;margin:6px 0 2px;color:var(--dim);font-size:13px}
.row{display:grid;grid-template-columns:1fr 1fr;gap:8px}
.btn{padding:10px 14px;border:0;border-radius:6px;background:#2a5;color:#fff;font-size:15px;margin:12px 6px 0 0}
.btn.sec{background:#444}.btn.del{background:#822}
#msg{margin-top:10px;min-height:18px}.err{color:var(--crit)}.okm{color:var(--ok)}
code{word-break:break-all;font-size:12px;color:#ccc}
</style></head><body>
<header><button data-t="live" class="on">Live</button><button data-t="cfg">Setări</button><button data-t="logs">Log-uri</button><button data-t="diag">AEM</button></header>

<section id="live" class="on"><div class="grid" id="cards"></div><div class="meta" id="meta"></div></section>

<section id="cfg">
<h3>Praguri alarmă</h3>
<table><thead><tr><th>Canal</th><th>Activ</th><th>Avert.</th><th>Critic</th><th>Hist.</th></tr></thead><tbody id="al"></tbody></table>
<div class="meta">Boost în bar relativ (1 bar ≈ 14.5 psi). λ și baterie mică: alarmă când valoarea SCADE sub prag.</div>

<h3>Display-uri</h3><div id="scr"></div>
<label><input type="checkbox" id="boost_psi"> Boost în psi (altfel bar)</label>

<h3>Pt100 (MAX31865)</h3>
<div class="row"><div><label>RREF (Ω)</label><input id="rtd_rref" type="number" step="0.1"></div><div><label>R0 (Ω)</label><input id="rtd_r0" type="number" step="0.1"></div></div>
<div class="row"><div><label>Corecție apă (°C)</label><input id="trim0" type="number" step="0.1"></div><div><label>Corecție ulei (°C)</label><input id="trim1" type="number" step="0.1"></div></div>
<div class="row"><div><label>Corecție aer (°C)</label><input id="trim2" type="number" step="0.1"></div><div></div></div>

<h3>Senzor presiune (Auber)</h3>
<div class="row"><div><label>V la presiune minimă</label><input id="boost_vmin" type="number" step="0.01"></div><div><label>V la presiune maximă</label><input id="boost_vmax" type="number" step="0.01"></div></div>
<div class="row"><div><label>Presiune minimă (bar)</label><input id="boost_pmin" type="number" step="0.01"></div><div><label>Presiune maximă (bar)</label><input id="boost_pmax" type="number" step="0.01"></div></div>
<div class="row"><div><label>Corecție zero (bar)</label><input id="boost_zero" type="number" step="0.005"></div><div><label><input type="checkbox" id="boost_ratio"> Ratiometric</label></div></div>

<h3>AEM 30-0310</h3>
<div class="row"><div><label>AFR la 0 V</label><input id="afr_0v" type="number" step="0.01"></div><div><label>AFR la 5 V</label><input id="afr_5v" type="number" step="0.01"></div></div>
<div class="row"><div><label>AFR stoichiometric</label><input id="afr_stoich" type="number" step="0.01"></div><div><label>Baud serial</label><input id="aem_baud" type="number"></div></div>
<div class="row"><div><label><input type="checkbox" id="aem_serial"> Citire serial RS232</label></div><div><label>Valoarea serială este</label><select id="aem_unit"><option value="0">automat</option><option value="1">AFR</option><option value="2">λ</option></select></div></div>

<h3>Divizoare de tensiune</h3>
<div class="row"><div><label>Factor semnale 5 V</label><input id="div_5v" type="number" step="0.0001"></div><div><label>Factor baterie</label><input id="div_vbat" type="number" step="0.0001"></div></div>

<h3>Wi-Fi și log</h3>
<div class="row"><div><label>Nume rețea</label><input id="wifi_ssid" maxlength="32"></div><div><label>Parolă nouă (gol = neschimbată)</label><input id="wifi_pass" type="password" maxlength="63"></div></div>
<div class="row"><div><label>Oprire Wi-Fi după (min, 0 = niciodată)</label><input id="wifi_idle_min" type="number"></div><div><label>Perioadă log (ms)</label><input id="log_ms" type="number"></div></div>
<label><input type="checkbox" id="log_on"> Log pe card activ</label>

<button class="btn" id="save">Salvează</button><button class="btn sec" id="reload">Reîncarcă</button><button class="btn del" id="reset">Valori implicite</button>
<div id="msg"></div>
</section>

<section id="logs"><div id="loglist"></div><button class="btn sec" id="logref">Reîncarcă</button></section>

<section id="diag">
<div class="meta">Ultimii octeți primiți pe RS232 de la AEM (hex). Folosiți pentru confirmarea formatului serial.</div>
<p><code id="raw"></code></p><div class="meta" id="rawtxt"></div>
<button class="btn sec" id="rawref">Reîncarcă</button>
</section>

<script>
const CH=[["ect","Apă","°C",1],["eot","Ulei","°C",1],["iat","Aer după IC","°C",1],["egt","EGT","°C",0],["boost","Boost","bar",2],["lambda","λ","",2],["vbat","Baterie","V",1]];
const AL=[["ect","Apă"],["eot","Ulei"],["iat","Aer"],["egt","EGT"],["boost","Boost (bar)"],["lambda","λ (jos)"],["vbat_low","Baterie mică"],["vbat_high","Baterie mare"]];
const FAULT={255:"cip absent",254:"în afara domeniului",253:"lipsă ref. 5 V"};
const $=id=>document.getElementById(id);
let cfg=null,psi=true;

document.querySelectorAll("header button").forEach(b=>b.onclick=()=>{
  document.querySelectorAll("header button,section").forEach(e=>e.classList.remove("on"));
  b.classList.add("on");$(b.dataset.t).classList.add("on");
  if(b.dataset.t=="cfg"&&!cfg)loadCfg();if(b.dataset.t=="logs")loadLogs();if(b.dataset.t=="diag")loadRaw();
});

$("cards").innerHTML=CH.map(c=>`<div class="card" id="c_${c[0]}"><div class="k">${c[1]}</div><div class="v">---</div><div class="f"></div></div>`).join("");

async function live(){
  try{
    const d=await (await fetch("/api/live")).json();
    for(const [k,,u,dec] of CH){
      const el=$("c_"+k),v=d.v[k],f=d.f[k];
      el.className="card l"+d.l[k];
      let t="---";
      if(v!==null){if(k=="boost"&&psi)t=(v*14.5038).toFixed(1)+" psi";else t=v.toFixed(dec)+(u?" "+u:"");
        if(k=="lambda"&&d.lambda_clip==1)t=">"+t;if(k=="lambda"&&d.lambda_clip==-1)t="<"+t;}
      el.querySelector(".v").textContent=t;
      el.querySelector(".f").textContent=v===null&&f?(FAULT[f]||"defect 0x"+f.toString(16)):"";
    }
    const src={0:"-",1:"serial",2:"analogic"}[d.lambda_src];
    $("meta").textContent=`λ: ${src} · AEM ${d.aem_v!==undefined?d.aem_v.toFixed(2)+" V":"-"} · ref. 5 V ${d.ref5v!==undefined?d.ref5v.toFixed(2)+" V":"-"} · SD ${d.sd?"OK ("+d.log+")":"EROARE"}`;
  }catch(e){$("meta").textContent="fără conexiune";}
}
fetch("/api/settings").then(r=>r.json()).then(c=>psi=c.boost_psi).catch(()=>{});
setInterval(live,500);live();

function opts(sel){return CH.map(c=>`<option value="${c[0]}"${c[0]==sel?" selected":""}>${c[1]}</option>`).join("")}
const NUM=["rtd_rref","rtd_r0","boost_vmin","boost_vmax","boost_pmin","boost_pmax","boost_zero","afr_0v","afr_5v","afr_stoich","aem_baud","div_5v","div_vbat","wifi_idle_min","log_ms"];
const BOOL=["boost_ratio","aem_serial","boost_psi","log_on"];

async function loadCfg(){
  cfg=await (await fetch("/api/settings")).json();psi=cfg.boost_psi;
  $("al").innerHTML=AL.map(([k,n])=>{const a=cfg.alarms[k];return `<tr><td>${n}</td><td><input type="checkbox" id="a_${k}_on"${a.on?" checked":""}></td><td><input id="a_${k}_warn" type="number" step="any" value="${a.warn}"></td><td><input id="a_${k}_crit" type="number" step="any" value="${a.crit}"></td><td><input id="a_${k}_hyst" type="number" step="any" value="${a.hyst}"></td></tr>`}).join("");
  $("scr").innerHTML=cfg.screens.map((s,i)=>`<div class="row"><div><label>Ecran ${i+1}: arc</label><select id="s${i}m">${opts(s.main)}</select></div><div><label>Ecran ${i+1}: secundar</label><select id="s${i}s">${opts(s.sub)}</select></div></div>`).join("");
  NUM.forEach(k=>$(k).value=cfg[k]);BOOL.forEach(k=>$(k).checked=cfg[k]);
  [0,1,2].forEach(i=>$("trim"+i).value=cfg.rtd_trim[i]);
  $("aem_unit").value=cfg.aem_unit;$("wifi_ssid").value=cfg.wifi_ssid;$("wifi_pass").value="";
}

function msg(t,ok){$("msg").textContent=t;$("msg").className=ok?"okm":"err";}

$("save").onclick=async()=>{
  const o={alarms:{},screens:[]};
  for(const [k] of AL)o.alarms[k]={on:$(`a_${k}_on`).checked,warn:+$(`a_${k}_warn`).value,crit:+$(`a_${k}_crit`).value,hyst:+$(`a_${k}_hyst`).value};
  for(let i=0;i<3;i++)o.screens.push({main:$(`s${i}m`).value,sub:$(`s${i}s`).value});
  NUM.forEach(k=>o[k]=+$(k).value);BOOL.forEach(k=>o[k]=$(k).checked);
  o.rtd_trim=[0,1,2].map(i=>+$("trim"+i).value);o.aem_unit=+$("aem_unit").value;o.wifi_ssid=$("wifi_ssid").value;
  if($("wifi_pass").value)o.wifi_pass=$("wifi_pass").value;
  const r=await fetch("/api/settings",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(o)});
  const d=await r.json();
  if(r.ok){msg("Salvat.",true);psi=o.boost_psi;if(o.wifi_pass||o.wifi_ssid!=cfg.wifi_ssid)msg("Salvat. Noul nume/parolă Wi-Fi se aplică la următoarea pornire a Wi-Fi.",true);cfg=null;loadCfg();}
  else msg("Eroare: "+d.error,false);
};
$("reload").onclick=loadCfg;
$("reset").onclick=async()=>{if(!confirm("Revii la valorile implicite?"))return;await fetch("/api/settings/reset",{method:"POST"});msg("Valori implicite restaurate.",true);loadCfg();};

async function loadLogs(){
  const d=await (await fetch("/api/logs")).json();
  d.files.sort((a,b)=>b.name.localeCompare(a.name));
  $("loglist").innerHTML=d.files.length?`<table>${d.files.map(f=>`<tr><td><a href="/api/logs/get?name=${f.name}" style="color:#9cf">${f.name}</a></td><td>${(f.size/1024).toFixed(0)} KB</td><td>${"/"+f.name==d.current?"în curs":`<button class="btn del" style="margin:0;padding:4px 8px" onclick="delLog('${f.name}')">Șterge</button>`}</td></tr>`).join("")}</table>`:"Niciun fișier.";
}
async function delLog(n){if(!confirm("Ștergi "+n+"?"))return;await fetch("/api/logs/delete?name="+n,{method:"POST"});loadLogs();}
$("logref").onclick=loadLogs;

async function loadRaw(){
  const d=await (await fetch("/api/aem/raw")).json();$("raw").textContent=d.hex||"(nimic primit)";
  $("rawtxt").textContent="Text: "+(d.hex||"").trim().split(" ").filter(x=>x).map(h=>{const c=parseInt(h,16);return c==13?"\\r":c==10?"\\n":(c>=32&&c<127?String.fromCharCode(c):"·")}).join("");
}
$("rawref").onclick=loadRaw;
</script></body></html>)HTML";
