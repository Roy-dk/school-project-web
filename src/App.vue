<script setup>
// This starter template is using Vue 3 <script setup> SFCs
// Check out https://v3.vuejs.org/api/sfc-script-setup.html#sfc-script-setup
import { onMounted, ref } from "vue";
import HelloWorld from "./components/HelloWorld.vue";
import Axios from "axios";
import * as echarts from "echarts";
import { context } from "ant-design-vue/lib/vc-image/src/PreviewGroup";
const value1 = ref({});
const temperature = ref("");
const humidity = ref("");
const date = ref([0,0,0,0,]);
const time = ref([0,0,0,0,]);
const adc_v = ref("");
const send_date_value = ref([0, 0, 0, 0]);
const send_time_value = ref([0, 0, 0]);
const send_temperature_value = ref(0);
const send_humidity_value = ref(0);
const cmd_value = ref("");
let dateFormat = "YYYY/MM/DD";
Axios.defaults.baseURL = "http://127.0.0.1:3001/api/";
var myChart;
var option;
let now = new Date();
let chart_data = [];
let value = 0;
let t = 0;
function randomData() {
  let now = new Date();
  value = value + Math.random() * 21 - 10;
  return {
    name: now.getSeconds(),
    value: [Math.round(value)],
  };
}
async function get_data() {
  let res = await Axios.get("/get_data", {});
  //console.log(res.data);
  temperature.value = res.data["temperature"][0];
  humidity.value = res.data["humidity"][0];
  date.value = res.data["date"];
  time.value = res.data["time"];
  adc_v.value = res.data["adc_v"][0];

  for (var i = 0; i < 5; i++) {
    let now = new Date();
    value = value + Math.random() * 21 - 10;
    chart_data.shift();
    t = t + 1;
    chart_data.push({
      name: t,
      value: [t, adc_v.value],
    });
  }

  myChart.setOption({
    series: [
      {
        data: chart_data,
      },
    ],
  });
}

async function send_cmd() {
  await Axios.post("/cmd", { cmd: cmd_value.value });
}

async function humidity_change() {
  await Axios.post("/cmd", {
    cmd: "alarm_humidity=" + send_humidity_value.value + ";",
  });
}
async function date_change() {
  await Axios.post("/cmd", {
    cmd:
      "date=" +
      send_date_value.value[0] +
      "," +
      send_date_value.value[1] +
      "," +
      send_date_value.value[2] +
      "," +
      send_date_value.value[3] +
      ";",
  });
}

async function time_change() {
  await Axios.post("/cmd", {
    cmd:
      "time=" +
      send_date_value.value[0] +
      "," +
      send_date_value.value[1] +
      "," +
      send_date_value.value[2] +
      ";",
  });
}
async function temperature_change() {
  await Axios.post("/cmd", {
    cmd: "alarm_temperature=" + send_temperature_value.value + ";",
  });
  //await Axios.post("/cmd", { cmd: "date=29,22,22,22;" });
}
async function mounted() {
  let res = await Axios.get("/opentcp", {});
  if (res.data.code == 1) {
    let chartDom = document.getElementById("main");
    myChart = echarts.init(chartDom);
    setInterval(get_data, 10);
  }
}

onMounted(mounted);
setTimeout(() => {
  let chartDom = document.getElementById("main");
  myChart = echarts.init(chartDom);

  let value = Math.random() * 1000;
  for (var i = 0; i < 1000; i++) {
    chart_data.push(randomData());
  }
  option = {
    title: {
      text: "波形",
    },
    tooltip: {
      trigger: "axis",
      formatter: function (params) {
        params = params[0];
        var date = new Date();
        return now.getSeconds();
      },
      axisPointer: {
        animation: false,
      },
    },
    xAxis: {
      type: "time",
      splitLine: {
        show: false,
      },
    },
    yAxis: {
      type: "value",
      boundaryGap: [0, "100%"],
      splitLine: {
        show: false,
      },
    },
    series: [
      {
        name: "Fake Data",
        type: "line",
        showSymbol: false,
        data: chart_data,
      },
    ],
  };
  setInterval(function () {}, 100);

  option && myChart.setOption(option);
}, 10);
</script>

<template>
  <div
    style="
      display: flex;
      flex-wrap: wrap;
      width: 100%;
      height: 100%;
      background-color: #fafafa;
    "
  >
    <div class="custom-card" style="margin: 20px; width: 200px">
      <a-button @click="mounted" type="primary" style="margin: 20px">
        打开tcp
      </a-button>
      <div>
        <a-input
          style="width: 180px"
          id="send_humidity_value"
          v-model:value="cmd_value"
        />
      </div>
      <div>
        <a-button @click="temperature_change" style="width:100%" type="primary">
          发送命令
        </a-button>
      </div>
    </div>
    <div class="custom-card" style="margin: 20px; width: 200px">
      <div>
        <h3>当前温度：{{ temperature }}</h3>
        <div>
          温度警报
          <a-input-number
            id="send_humidity_value"
            v-model:value="send_temperature_value"
          />
          <a-button @click="temperature_change" style="width:100%"  type="primary"> 发送 </a-button>
        </div>
      </div>
    </div>
    <div div class="custom-card" style="margin: 20px; width: 200px">
      <h3>当前湿度：{{ humidity }}</h3>
      <div>
        湿度警报

        <a-input-number
          id="send_humidity_value"
          v-model:value="send_humidity_value"
        />

        <a-button @click="humidity_change" style="width:100%"  type="primary"> 发送 </a-button>
      </div>
    </div>
    <div div class="custom-card" style="margin: 20px; width: 200px">
      <h3>日期：{{ date[0] + "年" + date[1] + "月" + date[2] + "日" }}</h3>
      <div>
        <a-button @click="date_change" style="width:100%"  type="primary"> 发送日期 </a-button>
        <a-input-number
          id="send_humidity_value"
          v-model:value="send_date_value[0]"
        />年
        <a-input-number
          id="send_humidity_value"
          v-model:value="send_date_value[1]"
        />月
        <a-input-number
          id="send_humidity_value"
          v-model:value="send_date_value[2]"
        />日
      </div>
    </div>
    <div div class="custom-card" style="margin: 20px; width: 200px">
      <h3>时间：{{ time[0] + "时" + time[1] + "分" + time[2] + "秒" }}</h3>
      <div>
        <a-button @click="time_change" style="width:100%"  type="primary"> 发送时间 </a-button>
        <a-input-number
          id="send_humidity_value"
          v-model:value="send_time_value[0]"
        />时
        <a-input-number
          id="send_humidity_value"
          v-model:value="send_time_value[1]"
        />分
        <a-input-number
          id="send_humidity_value"
          v-model:value="send_time_value[2]"
        />秒
      </div>
    </div>
    <div div class="custom-card" style="margin: 20px; width: 100%">
      <h3>电压：{{ adc_v }}</h3>
      <div id="main" style="height: 350px; width: 100%"></div>
    </div>
  </div>
</template>



<style>
#app {
  font-family: Avenir, Helvetica, Arial, sans-serif;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
  text-align: center;
  color: #2c3e50; 
}
.custom-card {
    margin: 10px;
    padding: 20px; 
    background-color: #fff;
    border-radius: 12px;
    box-shadow: 0px 20px 27px rgb(0 0 0 / 10%);
    transition: all 0.3s;

}
a-button{
  margin: 10px;
}
</style>
